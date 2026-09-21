#include "hvac_zones.h"

#include <WiFi.h>

#include "data_mutex.h"
#include "hubitat_client.h"
#include "secrets.h"

namespace {
constexpr unsigned long FETCH_INTERVAL_MS = 30UL * 1000UL;
// IDs and labels come from secrets.h (git-ignored) rather than being
// hardcoded here, so your own zone names/device IDs never end up in
// version control -- see HVAC_ZONE_IDS/HVAC_ZONE_LABELS in secrets.h.example.
constexpr const char *ZONE_IDS[HvacZonesManager::NUM_ZONES] = HVAC_ZONE_IDS;
constexpr const char *ZONE_LABELS[HvacZonesManager::NUM_ZONES] = HVAC_ZONE_LABELS;
}  // namespace

void HvacZonesManager::loop() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    unsigned long now = millis();
    if (lastFetchMs != 0 && (now - lastFetchMs) < FETCH_INTERVAL_MS) {
        return;
    }
    lastFetchMs = now;
    fetchNow();
}

bool HvacZonesManager::fetchNow() {
    bool anyOk = false;
    for (int i = 0; i < NUM_ZONES; i++) {
        JsonDocument doc;
        if (!hubitatGetDevice(ZONE_IDS[i], doc)) {
            continue;
        }

        HvacZoneData z;
        z.label = ZONE_LABELS[i];
        z.mode = hubitatAttrString(doc, "thermostatMode");
        z.currentTempF = hubitatAttrFloat(doc, "temperature", 0);
        z.coolingSetpointF = hubitatAttrFloat(doc, "coolingSetpoint", 0);
        z.heatingSetpointF = hubitatAttrFloat(doc, "heatingSetpoint", 0);
        z.fanMode = hubitatAttrString(doc, "thermostatFanMode");
        z.operatingState = hubitatAttrString(doc, "thermostatOperatingState");
        z.valid = true;

        DataLock lock;
        current[i] = z;
        anyOk = true;
    }
    return anyOk;
}

HvacZoneData HvacZonesManager::data(int index) const {
    DataLock lock;
    if (index < 0 || index >= NUM_ZONES) {
        return HvacZoneData();
    }
    return current[index];
}

bool HvacZonesManager::anyActivelyHeatingOrCooling() const {
    for (int i = 0; i < NUM_ZONES; i++) {
        HvacZoneData z = data(i);
        if (z.valid && (z.operatingState == "heating" || z.operatingState == "cooling")) {
            return true;
        }
    }
    return false;
}

bool HvacZonesManager::sendMode(int index, const char *command) {
    if (index < 0 || index >= NUM_ZONES) {
        return false;
    }
    if (!hubitatSendCommand(ZONE_IDS[index], command)) {
        return false;
    }
    DataLock lock;
    current[index].mode = command;
    return true;
}

bool HvacZonesManager::adjustSetpoint(int index, float deltaF) {
    if (index < 0 || index >= NUM_ZONES) {
        return false;
    }

    HvacZoneData zone = data(index);
    bool isCooling = (zone.mode == "cool");
    bool isHeating = (zone.mode == "heat" || zone.mode == "emergency heat");
    if (!isCooling && !isHeating) {
        return false;  // off/dry/auto -- no single setpoint to adjust
    }

    float current_ = isCooling ? zone.coolingSetpointF : zone.heatingSetpointF;
    float next = constrain(current_ + deltaF, 60.0f, 85.0f);
    const char *command = isCooling ? "setCoolingSetpoint" : "setHeatingSetpoint";
    if (!hubitatSendCommand(ZONE_IDS[index], command, String(next, 1))) {
        return false;
    }

    DataLock lock;
    if (isCooling) {
        current[index].coolingSetpointF = next;
    } else {
        current[index].heatingSetpointF = next;
    }
    return true;
}

bool HvacZonesManager::cycleFanMode(int index) {
    if (index < 0 || index >= NUM_ZONES) {
        return false;
    }
    HvacZoneData zone = data(index);
    const char *nextCommand;
    const char *nextMode;
    if (zone.fanMode == "auto") {
        nextCommand = "fanOn";
        nextMode = "on";
    } else if (zone.fanMode == "on") {
        nextCommand = "fanCirculate";
        nextMode = "circulate";
    } else {
        nextCommand = "fanAuto";
        nextMode = "auto";
    }
    if (!hubitatSendCommand(ZONE_IDS[index], nextCommand)) {
        return false;
    }
    DataLock lock;
    current[index].fanMode = nextMode;
    return true;
}
