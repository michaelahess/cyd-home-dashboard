#include "weather.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "data_mutex.h"
#include "secrets.h"

namespace {
constexpr unsigned long FETCH_INTERVAL_MS = 15UL * 60UL * 1000UL;
constexpr unsigned long RETRY_INTERVAL_MS = 60UL * 1000UL;

const char *WEEKDAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Sakamoto's algorithm; avoids pulling in a date library for one lookup.
int dayOfWeek(int y, int m, int d) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) {
        y -= 1;
    }
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;  // 0 = Sunday
}

String weekdayAbbrev(const char *isoDate) {
    int y = 0, m = 0, d = 0;
    sscanf(isoDate, "%d-%d-%d", &y, &m, &d);
    return WEEKDAY_NAMES[dayOfWeek(y, m, d)];
}

String urlEncodeSlashes(const String &value) {
    String out;
    for (char c : value) {
        if (c == '/') {
            out += "%2F";
        } else {
            out += c;
        }
    }
    return out;
}
}  // namespace

namespace {
const char *disconnectReasonText(uint8_t reason) {
    switch (reason) {
        case 2:
            return "AUTH_EXPIRE";
        case 15:
            return "4WAY_HANDSHAKE_TIMEOUT (usually wrong password)";
        case 201:
            return "NO_AP_FOUND (SSID not seen -- wrong name, out of range, or 5GHz-only)";
        case 202:
            return "AUTH_FAIL (usually wrong password)";
        case 203:
            return "ASSOC_FAIL";
        case 204:
            return "HANDSHAKE_TIMEOUT";
        default:
            return "see https://github.com/espressif/esp-idf/blob/master/components/esp_wifi/include/esp_wifi_types_generic.h for this code";
    }
}
}  // namespace

void WeatherManager::begin() {
    if (ZIP_CODE[0] == '\0') {
        // No zip given -- use the lat/lon constants directly, no network
        // round-trip needed to resolve them.
        lat = LATITUDE;
        lon = LONGITUDE;
        locationResolved = true;
    }

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        switch (event) {
            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                Serial.printf("WiFi disconnect event, reason=%u (%s)\n", info.wifi_sta_disconnected.reason,
                               disconnectReasonText(info.wifi_sta_disconnected.reason));
                break;
            case ARDUINO_EVENT_WIFI_STA_CONNECTED:
                Serial.println("WiFi STA associated with AP");
                break;
            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                Serial.println("WiFi got IP");
                break;
            default:
                break;
        }
    });

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    configTzTime(TZ_POSIX, "pool.ntp.org");
    Serial.printf("WiFi: connecting to \"%s\"...\n", WIFI_SSID);
}

bool WeatherManager::wifiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

namespace {
const char *wifiStatusText(wl_status_t status) {
    switch (status) {
        case WL_IDLE_STATUS:
            return "IDLE";
        case WL_NO_SSID_AVAIL:
            return "NO_SSID_AVAIL (SSID not seen -- wrong name, or 5GHz-only network?)";
        case WL_SCAN_COMPLETED:
            return "SCAN_COMPLETED";
        case WL_CONNECTED:
            return "CONNECTED";
        case WL_CONNECT_FAILED:
            return "CONNECT_FAILED (likely wrong password)";
        case WL_CONNECTION_LOST:
            return "CONNECTION_LOST";
        case WL_DISCONNECTED:
            return "DISCONNECTED";
        default:
            return "UNKNOWN";
    }
}
}  // namespace

void WeatherManager::loop() {
    static wl_status_t lastLoggedStatus = static_cast<wl_status_t>(-1);
    static uint32_t lastStatusLogMs = 0;
    wl_status_t status = WiFi.status();
    uint32_t now32 = millis();
    if (status != lastLoggedStatus || now32 - lastStatusLogMs > 5000) {
        Serial.printf("WiFi status: %s\n", wifiStatusText(status));
        lastLoggedStatus = status;
        lastStatusLogMs = now32;
    }

    if (!wifiConnected()) {
        return;
    }

    static bool ipLogged = false;
    if (!ipLogged) {
        ipLogged = true;
        Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
    }

    if (!locationResolved) {
        unsigned long nowMs = millis();
        if (lastLocationAttemptMs == 0 || (nowMs - lastLocationAttemptMs) >= RETRY_INTERVAL_MS) {
            lastLocationAttemptMs = nowMs;
            locationResolved = resolveLocation();
        }
        if (!locationResolved) {
            return;  // no point fetching weather for an unresolved location yet
        }
    }

    unsigned long now = millis();
    unsigned long interval = current.valid ? FETCH_INTERVAL_MS : RETRY_INTERVAL_MS;
    if (firstFetchDone && (now - lastFetchMs) < interval) {
        return;
    }

    firstFetchDone = true;
    lastFetchMs = now;
    fetchNow();
}

bool WeatherManager::resolveLocation() {
    // zippopotam.us: free, no API key, plain HTTP, and covers postal codes
    // for dozens of countries (not just the US) via a country-code path
    // segment -- a reasonable default so this project isn't US-only for
    // anyone who fills in ZIP_CODE instead of LATITUDE/LONGITUDE directly.
    String url = String("http://api.zippopotam.us/") + ZIP_COUNTRY + "/" + ZIP_CODE;
    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    if (code != 200) {
        Serial.printf("Zip lookup failed for %s/%s, HTTP %d\n", ZIP_COUNTRY, ZIP_CODE, code);
        http.end();
        return false;
    }
    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    JsonArray places = doc["places"];
    if (err || places.size() == 0) {
        Serial.printf("Zip lookup parse failed for %s/%s: %s\n", ZIP_COUNTRY, ZIP_CODE, err.c_str());
        return false;
    }

    JsonObject place = places[0];
    lat = atof(place["latitude"] | "0");
    lon = atof(place["longitude"] | "0");
    Serial.printf("Zip %s/%s resolved to %s, lat=%.5f lon=%.5f\n", ZIP_COUNTRY, ZIP_CODE,
                  (place["place name"] | "?"), lat, lon);
    return true;
}

bool WeatherManager::fetchNow() {
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 5) +
                 "&longitude=" + String(lon, 5) +
                 "&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m"
                 "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max"
                 "&temperature_unit=fahrenheit&wind_speed_unit=mph&forecast_days=5" +
                 "&timezone=" + urlEncodeSlashes(TZ_IANA);

    WiFiClientSecure client;
    client.setInsecure();  // public weather data; skipping cert pinning is an acceptable tradeoff here
    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();
    if (httpCode != 200) {
        Serial.printf("Weather fetch failed, HTTP %d\n", httpCode);
        http.end();
        return false;
    }
    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("Weather JSON parse failed: %s\n", err.c_str());
        return false;
    }

    WeatherData next;
    next.currentTempF = doc["current"]["temperature_2m"] | 0.0f;
    next.feelsLikeF = doc["current"]["apparent_temperature"] | 0.0f;
    next.humidity = doc["current"]["relative_humidity_2m"] | 0;
    next.weatherCode = doc["current"]["weather_code"] | 0;
    next.windSpeedMph = doc["current"]["wind_speed_10m"] | 0.0f;

    JsonArray times = doc["daily"]["time"];
    JsonArray codes = doc["daily"]["weather_code"];
    JsonArray highs = doc["daily"]["temperature_2m_max"];
    JsonArray lows = doc["daily"]["temperature_2m_min"];
    JsonArray precip = doc["daily"]["precipitation_probability_max"];

    int count = min(static_cast<int>(times.size()), WeatherData::NUM_DAYS);
    for (int i = 0; i < count; i++) {
        DailyForecast &day = next.daily[i];
        day.weatherCode = codes[i] | 0;
        day.tempMaxF = highs[i] | 0.0f;
        day.tempMinF = lows[i] | 0.0f;
        day.precipProbability = precip[i] | 0;
        day.dayLabel = (i == 0) ? "Today" : weekdayAbbrev(times[i].as<const char *>());
    }

    next.valid = true;
    {
        DataLock lock;
        current = next;
    }
    Serial.printf("Weather updated: %.1fF, %s\n", next.currentTempF, weatherCodeToText(next.weatherCode));
    return true;
}

WeatherData WeatherManager::data() const {
    DataLock lock;
    return current;
}

const char *weatherCodeToText(int code) {
    switch (code) {
        case 0:
            return "Clear";
        case 1:
            return "Mainly Clear";
        case 2:
            return "Partly Cloudy";
        case 3:
            return "Overcast";
        case 45:
        case 48:
            return "Fog";
        case 51:
        case 53:
        case 55:
            return "Drizzle";
        case 56:
        case 57:
            return "Freezing Drizzle";
        case 61:
        case 63:
        case 65:
            return "Rain";
        case 66:
        case 67:
            return "Freezing Rain";
        case 71:
        case 73:
        case 75:
        case 77:
            return "Snow";
        case 80:
        case 81:
        case 82:
            return "Rain Showers";
        case 85:
        case 86:
            return "Snow Showers";
        case 95:
            return "Thunderstorm";
        case 96:
        case 99:
            return "Thunderstorm w/ Hail";
        default:
            return "Unknown";
    }
}

WeatherIcon weatherCodeToIcon(int code) {
    if (code == 0 || code == 1) {
        return WeatherIcon::SUN;
    }
    if (code == 2) {
        return WeatherIcon::PARTLY_CLOUDY;
    }
    if (code == 3) {
        return WeatherIcon::CLOUDY;
    }
    if (code == 45 || code == 48) {
        return WeatherIcon::FOG;
    }
    if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) {
        return WeatherIcon::RAIN;
    }
    if ((code >= 71 && code <= 77) || code == 85 || code == 86) {
        return WeatherIcon::SNOW;
    }
    if (code == 95 || code == 96 || code == 99) {
        return WeatherIcon::STORM;
    }
    return WeatherIcon::CLOUDY;
}
