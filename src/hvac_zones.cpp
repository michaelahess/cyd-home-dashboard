#include "hvac_zones.h"

#include <WiFi.h>

#include "data_mutex.h"
#include "ha_client.h"
#include "secrets.h"

namespace {
constexpr unsigned long FETCH_INTERVAL_MS = 30UL * 1000UL;
// HA climate entity IDs and labels come from secrets.h (git-ignored) rather
// than being hardcoded here, so your own zone names never end up in version
// control -- see HVAC_ZONE_ENTITIES/HVAC_ZONE_LABELS in secrets.h.example.
// An empty entity ID leaves that zone showing "-".
constexpr const char *ZONE_ENTITIES[HvacZonesManager::NUM_ZONES] = HVAC_ZONE_ENTITIES;
constexpr const char *ZONE_LABELS[HvacZonesManager::NUM_ZONES] = HVAC_ZONE_LABELS;

// HA's hvac_mode/hvac_action vocabulary differs slightly from what the UI
// (hvac_page.cpp) was written against; translate here so pages don't change.
String uiMode(const String &haMode) {
    if (haMode == "heat_cool") {
        return "auto";
    }
    if (haMode == "fan_only") {
        return "fan";
    }
    return haMode;
}

String uiAction(const String &haAction) {
    if (haAction == "fan") {
        return "fan only";
    }
    return haAction;
}

bool csvContains(const String &csv, const String &item) {
    return ("," + csv + ",").indexOf("," + item + ",") >= 0;
}

bool callClimate(const char *service, const char *entity, const char *key, const String &value) {
    JsonDocument body;
    body["entity_id"] = entity;
    body[key] = value;
    return haCallService("climate", service, body);
}
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
    // One POST returning every zone as an array of the climate entity's
    // state + attributes (null for an unconfigured zone).
    String tpl = "[";
    for (int i = 0; i < NUM_ZONES; i++) {
        if (ZONE_ENTITIES[i][0] == '\0') {
            tpl += "null";
        } else {
            String e = ZONE_ENTITIES[i];
            tpl += "{{ {'s': states('" + e + "'), 'ct': state_attr('" + e + "', 'current_temperature'), 'sp': state_attr('" +
                   e + "', 'temperature'), 'lo': state_attr('" + e + "', 'target_temp_low'), 'hi': state_attr('" + e +
                   "', 'target_temp_high'), 'fan': state_attr('" + e + "', 'fan_mode'), 'fans': state_attr('" + e +
                   "', 'fan_modes'), 'hms': state_attr('" + e + "', 'hvac_modes'), 'act': state_attr('" + e +
                   "', 'hvac_action')} | tojson }}";
        }
        if (i < NUM_ZONES - 1) {
            tpl += ", ";
        }
    }
    tpl += "]";
    JsonDocument doc;
    if (!haFetchTemplate(tpl, doc)) {
        return false;
    }

    bool anyOk = false;
    for (int i = 0; i < NUM_ZONES; i++) {
        JsonObject o = doc[i];
        String state = o["s"] | "";
        if (o.isNull() || state.length() == 0 || state == "unavailable" || state == "unknown") {
            continue;
        }

        HvacZoneData z;
        z.label = ZONE_LABELS[i];
        z.mode = uiMode(state);
        z.currentTempF = o["ct"] | 0.0f;
        // "temperature" is the single target in heat/cool; "target_temp_low/
        // high" are the pair in heat_cool. Take whichever the entity has.
        float single = o["sp"] | 0.0f;
        z.coolingSetpointF = o["hi"] | single;
        z.heatingSetpointF = o["lo"] | single;
        z.fanMode = o["fan"] | "";
        z.operatingState = uiAction(o["act"] | "");
        z.valid = true;

        String fanModes, hvacModes;
        for (JsonVariant m : o["fans"].as<JsonArray>()) {
            fanModes += (fanModes.length() ? "," : "") + m.as<String>();
        }
        for (JsonVariant m : o["hms"].as<JsonArray>()) {
            hvacModes += (hvacModes.length() ? "," : "") + m.as<String>();
        }

        DataLock lock;
        current[i] = z;
        fanModesCsv[i] = fanModes;
        hvacModesCsv[i] = hvacModes;
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
    String haMode = command;
    if (haMode == "auto") {
        // Mitsubishi-style units expose "heat_cool"; others a plain "auto".
        DataLock lock;
        if (csvContains(hvacModesCsv[index], "heat_cool")) {
            haMode = "heat_cool";
        }
    }
    if (!callClimate("set_hvac_mode", ZONE_ENTITIES[index], "hvac_mode", haMode)) {
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
    // Only reachable in cool/heat mode, where the entity has one target.
    JsonDocument body;
    body["entity_id"] = ZONE_ENTITIES[index];
    body["temperature"] = next;
    if (!haCallService("climate", "set_temperature", body)) {
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
    // Step through whatever fan modes this entity reports, in its own order.
    String modes, currentMode;
    {
        DataLock lock;
        modes = fanModesCsv[index];
        currentMode = current[index].fanMode;
    }
    if (modes.length() == 0) {
        return false;
    }
    constexpr int MAX_FAN_MODES = 8;
    String list[MAX_FAN_MODES];
    int n = 0, start = 0;
    for (int i = 0; i <= (int)modes.length() && n < MAX_FAN_MODES; i++) {
        if (i == (int)modes.length() || modes[i] == ',') {
            list[n++] = modes.substring(start, i);
            start = i + 1;
        }
    }
    int cur = -1;
    for (int i = 0; i < n; i++) {
        if (list[i] == currentMode) {
            cur = i;
        }
    }
    String next = list[(cur + 1) % n];
    if (!callClimate("set_fan_mode", ZONE_ENTITIES[index], "fan_mode", next)) {
        return false;
    }
    DataLock lock;
    current[index].fanMode = next;
    return true;
}
