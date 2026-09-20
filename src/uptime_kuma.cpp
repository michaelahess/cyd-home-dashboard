#include "uptime_kuma.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "data_mutex.h"
#include "secrets.h"

namespace {
constexpr unsigned long STATUS_FETCH_INTERVAL_MS = 60UL * 1000UL;

bool httpGet(const String &url, String &outPayload) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url);
    int code = http.GET();
    if (code != 200) {
        Serial.printf("Uptime Kuma GET %s failed, HTTP %d\n", url.c_str(), code);
        http.end();
        return false;
    }
    outPayload = http.getString();
    http.end();
    return true;
}
}  // namespace

void UptimeKumaManager::begin() {
    // Real fetch happens lazily from loop() once Wi-Fi is up; nothing to
    // do here since begin() runs before the network connects.
}

bool UptimeKumaManager::configured() {
    return UPTIME_KUMA_BASE_URL[0] != '\0' && UPTIME_KUMA_SLUG[0] != '\0';
}

bool UptimeKumaManager::fetchConfig() {
    String url = String(UPTIME_KUMA_BASE_URL) + "/api/status-page/" + UPTIME_KUMA_SLUG;
    String payload;
    if (!httpGet(url, payload)) {
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("Uptime Kuma config parse failed: %s\n", err.c_str());
        return false;
    }

    DataLock lock;
    numMonitors = 0;
    for (JsonObject group : doc["publicGroupList"].as<JsonArray>()) {
        for (JsonObject monitor : group["monitorList"].as<JsonArray>()) {
            if (numMonitors >= MAX_MONITORS) {
                break;
            }
            monitors[numMonitors].id = monitor["id"] | 0;
            String name = monitor["name"] | "";
            name.trim();
            monitors[numMonitors].name = name;
            monitors[numMonitors].up = true;
            numMonitors++;
        }
    }
    haveConfig = (numMonitors > 0);
    return haveConfig;
}

bool UptimeKumaManager::fetchStatuses() {
    // The status-page heartbeat endpoint returns ~125KB (100 history
    // entries per monitor) and repeatedly failed to parse reliably on this
    // device (tried buffering as a String, then stream-parsing with a
    // filter -- both hit truncation/chunked-encoding issues). Each
    // monitor's individual badge endpoint is ~1KB plain text containing
    // "Status: Up" or "Status: Down", which is simple and robust even if
    // it means one small request per monitor instead of one big one.
    bool anyOk = false;
    for (int i = 0; i < numMonitors; i++) {
        String url = String(UPTIME_KUMA_BASE_URL) + "/api/badge/" + String(monitors[i].id) + "/status";
        String payload;
        if (httpGet(url, payload)) {
            bool up = payload.indexOf("Status: Up") >= 0;
            DataLock lock;
            monitors[i].up = up;
            anyOk = true;
        }
        // On a failed request, keep that monitor's last-known state rather
        // than guessing.
    }
    return anyOk;
}

void UptimeKumaManager::loop() {
    if (!configured() || WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (!haveConfig) {
        fetchConfig();
        return;
    }

    unsigned long now = millis();
    if (lastStatusFetchMs != 0 && (now - lastStatusFetchMs) < STATUS_FETCH_INTERVAL_MS) {
        return;
    }
    lastStatusFetchMs = now;
    fetchStatuses();
}

bool UptimeKumaManager::configLoaded() const {
    DataLock lock;
    return haveConfig;
}

int UptimeKumaManager::monitorCount() const {
    DataLock lock;
    return numMonitors;
}

int UptimeKumaManager::upCount() const {
    DataLock lock;
    int n = 0;
    for (int i = 0; i < numMonitors; i++) {
        if (monitors[i].up) {
            n++;
        }
    }
    return n;
}

int UptimeKumaManager::downCount() const {
    // Not implemented via numMonitors - upCount(): both would take
    // DataLock, and it's a plain (non-recursive) mutex, so nesting the
    // calls would deadlock. Counts independently under its own lock instead.
    DataLock lock;
    int down = 0;
    for (int i = 0; i < numMonitors; i++) {
        if (!monitors[i].up) {
            down++;
        }
    }
    return down;
}

String UptimeKumaManager::downName(int index) const {
    DataLock lock;
    int seen = 0;
    for (int i = 0; i < numMonitors; i++) {
        if (!monitors[i].up) {
            if (seen == index) {
                return monitors[i].name;
            }
            seen++;
        }
    }
    return "";
}
