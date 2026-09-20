#include "windows_status.h"

#include <WiFi.h>

#include "data_mutex.h"
#include "hubitat_client.h"

namespace {
constexpr unsigned long FETCH_INTERVAL_MS = 30UL * 1000UL;
constexpr int MAX_SENSORS = 16;

// 8 window/patio-door contact sensors, confirmed via /devices: standard
// ContactSensor capability, "contact" attribute is "open"/"closed".
constexpr int NUM_SENSORS = 8;
constexpr const char *SENSOR_IDS[MAX_SENSORS] = {
    "1350",  // MBR - CT - Patio Door
    "1193",  // RRM - CT - Window
    "4330",  // LVR - CT - S Wnd
    "4331",  // LVR - CT - N Wnd
    "4332",  // MBR - CT - W Wnd
    "4334",  // BB - CT - Window
    "3407",  // RBN - CT - Patio Door
    "4346",  // MBT - CT - Window
};

bool sensorOpen[MAX_SENSORS] = {};
unsigned long lastFetchMs = 0;
}  // namespace

bool WindowsStatusManager::fetchNow() {
    bool anyOk = false;
    for (int i = 0; i < NUM_SENSORS; i++) {
        JsonDocument doc;
        if (!hubitatGetDevice(SENSOR_IDS[i], doc)) {
            continue;
        }
        String contact = hubitatAttrString(doc, "contact");  // "open" / "closed"
        DataLock lock;
        sensorOpen[i] = (contact == "open");
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
