#include "windows_status.h"

#include <WiFi.h>

#include "data_mutex.h"
#include "hubitat_client.h"

namespace {
constexpr unsigned long FETCH_INTERVAL_MS = 30UL * 1000UL;
constexpr int MAX_SENSORS = 16;

// Hubitat device IDs for your window/door contact sensors -- standard
// ContactSensor capability, "contact" attribute reports open/closed.
// Edit this list (and NUM_SENSORS) for your own devices; find IDs via
// Hubitat's Maker API app page or its /devices endpoint.
constexpr int NUM_SENSORS = 8;
constexpr const char *SENSOR_IDS[MAX_SENSORS] = {
    "1350", "1193", "4330", "4331", "4332", "4334", "3407", "4346",
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
        String contact = hubitatAttrString(doc, "contact");
        String normalized = contact;
        normalized.trim();
        normalized.toLowerCase();
        // Logged because different Hubitat drivers have been seen to report
        // this attribute with different casing/whitespace -- if a sensor
        // never seems to register as open, check here first for what it's
        // actually sending before assuming the sensor itself is at fault.
        Serial.printf("Window sensor %s: contact=\"%s\"\n", SENSOR_IDS[i], contact.c_str());
        DataLock lock;
        sensorOpen[i] = (normalized == "open");
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
