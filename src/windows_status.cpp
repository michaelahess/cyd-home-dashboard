#include "windows_status.h"

#include <WiFi.h>

#include "data_mutex.h"
#include "ha_client.h"
#include "secrets.h"

namespace {
constexpr unsigned long FETCH_INTERVAL_MS = 30UL * 1000UL;
constexpr int MAX_SENSORS = 16;

// Home Assistant binary_sensor entity IDs for your window/door contact
// sensors ("on" = open). Comes from secrets.h (git-ignored) rather than
// being hardcoded here -- see NUM_WINDOW_SENSORS/WINDOW_SENSOR_ENTITIES in
// secrets.h.example. Leaving NUM_WINDOW_SENSORS at 0 disables this feature
// entirely.
constexpr int NUM_SENSORS = NUM_WINDOW_SENSORS;
constexpr const char *SENSOR_ENTITIES[MAX_SENSORS] = WINDOW_SENSOR_ENTITIES;

bool sensorOpen[MAX_SENSORS] = {};
unsigned long lastFetchMs = 0;
}  // namespace

bool WindowsStatusManager::fetchNow() {
    // One POST returning every sensor's raw HA state as {"s0": "on", ...}.
    String tpl = "{";
    for (int i = 0; i < NUM_SENSORS; i++) {
        tpl += "\"s" + String(i) + "\": \"{{ states('" + SENSOR_ENTITIES[i] + "') }}\"";
        if (i < NUM_SENSORS - 1) {
            tpl += ", ";
        }
    }
    tpl += "}";
    JsonDocument doc;
    if (!haFetchTemplate(tpl, doc)) {
        return false;
    }

    bool anyOk = false;
    for (int i = 0; i < NUM_SENSORS; i++) {
        String state = doc["s" + String(i)] | "";
        // Logged so a sensor that never seems to register as open can be
        // checked against what HA is actually reporting for it.
        Serial.printf("Window sensor %s: state=\"%s\"\n", SENSOR_ENTITIES[i], state.c_str());
        if (state != "on" && state != "off") {
            continue;  // unavailable/unknown -- keep the last known value
        }
        DataLock lock;
        sensorOpen[i] = (state == "on");
        anyOk = true;
    }
    return anyOk;
}

void WindowsStatusManager::loop() {
    if (NUM_SENSORS == 0 || WiFi.status() != WL_CONNECTED) {
        return;
    }
    unsigned long now = millis();
    if (lastFetchMs != 0 && (now - lastFetchMs) < FETCH_INTERVAL_MS) {
        return;
    }
    lastFetchMs = now;
    fetchNow();
}

bool WindowsStatusManager::hasSensors() const {
    return NUM_SENSORS > 0;
}

bool WindowsStatusManager::anyOpen() const {
    DataLock lock;
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (sensorOpen[i]) {
            return true;
        }
    }
    return false;
}

bool WindowsStatusManager::allClosed() const {
    if (NUM_SENSORS == 0) {
        return false;
    }
    DataLock lock;
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (sensorOpen[i]) {
            return false;
        }
    }
    return true;
}
