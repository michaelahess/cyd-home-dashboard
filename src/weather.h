#pragma once

#include <Arduino.h>

struct DailyForecast {
    String dayLabel;
    int weatherCode = 0;
    float tempMaxF = 0;
    float tempMinF = 0;
    int precipProbability = 0;
};

struct WeatherData {
    bool valid = false;
    float currentTempF = 0;
    float feelsLikeF = 0;
    int humidity = 0;
    int weatherCode = 0;
    float windSpeedMph = 0;

    static constexpr int NUM_DAYS = 5;
    DailyForecast daily[NUM_DAYS];
};

// Owns Wi-Fi + NTP + periodic Open-Meteo fetches. Pages read the cached
// WeatherData; nothing here blocks a page from drawing with stale/no data.
class WeatherManager {
public:
    // Resolves ZIP_CODE (secrets.h) to a lat/lon via a free zip-lookup API
    // if one was given, otherwise uses LATITUDE/LONGITUDE directly. Safe to
    // call before Wi-Fi is up -- the zip lookup itself happens lazily from
    // the first loop() once connected.
    void begin();

    // Call every main-loop iteration. Cheap no-op most of the time; does a
    // blocking HTTPS fetch roughly every 15 minutes (and once on first
    // successful Wi-Fi connection).
    void loop();

    // Returns a locked copy -- safe to call from the render loop while the
    // background task may be mid-write to `current`.
    WeatherData data() const;
    bool wifiConnected() const;

private:
    WeatherData current;
    unsigned long lastFetchMs = 0;
    bool firstFetchDone = false;

    double lat = 0;
    double lon = 0;
    bool locationResolved = false;
    unsigned long lastLocationAttemptMs = 0;

    bool resolveLocation();
    bool fetchNow();
};

// Maps an Open-Meteo WMO weather_code to a short human-readable condition.
const char *weatherCodeToText(int code);

// Coarse icon category for a WMO weather_code, for pages to draw a simple
// hand-drawn glyph from (no bitmap assets).
enum class WeatherIcon {
    SUN,
    PARTLY_CLOUDY,
    CLOUDY,
    FOG,
    RAIN,
    SNOW,
    STORM,
};

WeatherIcon weatherCodeToIcon(int code);
