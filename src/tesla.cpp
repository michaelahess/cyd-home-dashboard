#include "tesla.h"

#include <WiFi.h>

#include "data_mutex.h"
#include "ha_client.h"
#include "secrets.h"

namespace {
constexpr unsigned long FETCH_INTERVAL_MS = 30UL * 1000UL;
// Names and HA entity IDs come from secrets.h (git-ignored) rather than
// being hardcoded here -- see the TESLA_* macros in secrets.h.example. An
// empty entity ID means "not available in HA": that field is skipped.
constexpr const char *VEHICLE_NAMES[TeslaManager::NUM_VEHICLES] = TESLA_VEHICLE_NAMES;
constexpr const char *BATTERY_ENTITIES[TeslaManager::NUM_VEHICLES] = TESLA_BATTERY_ENTITIES;
constexpr const char *INSIDE_TEMP_ENTITIES[TeslaManager::NUM_VEHICLES] = TESLA_INSIDE_TEMP_ENTITIES;
constexpr const char *OUTSIDE_TEMP_ENTITIES[TeslaManager::NUM_VEHICLES] = TESLA_OUTSIDE_TEMP_ENTITIES;
constexpr const char *LOCK_ENTITIES[TeslaManager::NUM_VEHICLES] = TESLA_LOCK_ENTITIES;
constexpr const char *STATE_ENTITIES[TeslaManager::NUM_VEHICLES] = TESLA_STATE_ENTITIES;
constexpr const char *PRESENCE_ENTITIES[TeslaManager::NUM_VEHICLES] = TESLA_PRESENCE_ENTITIES;

// Jinja for one field: its HA state as a JSON string, or null if the entity
// isn't configured. Missing/unavailable states are filtered in C++.
String fieldTemplate(const char *key, const char *entity) {
    String t = "\"" + String(key) + "\": ";
    t += entity[0] ? "{{ states('" + String(entity) + "') | tojson }}" : "null";
    return t;
}

// HA reports "unavailable"/"unknown" for a dead entity; treat as no data.
bool usable(const String &s) {
    return s.length() > 0 && s != "unavailable" && s != "unknown";
}
}  // namespace

void TeslaManager::loop() {
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

bool TeslaManager::fetchNow() {
    // One POST returning every vehicle as an array of raw-state objects.
    String tpl = "[";
    for (int i = 0; i < NUM_VEHICLES; i++) {
        tpl += "{" + fieldTemplate("bat", BATTERY_ENTITIES[i]) + ", " +
               fieldTemplate("in", INSIDE_TEMP_ENTITIES[i]) + ", " +
               fieldTemplate("out", OUTSIDE_TEMP_ENTITIES[i]) + ", " + fieldTemplate("lock", LOCK_ENTITIES[i]) +
               ", " + fieldTemplate("state", STATE_ENTITIES[i]) + ", " +
               fieldTemplate("pres", PRESENCE_ENTITIES[i]) + "}";
        if (i < NUM_VEHICLES - 1) {
            tpl += ", ";
        }
    }
    tpl += "]";
    JsonDocument doc;
    if (!haFetchTemplate(tpl, doc)) {
        return false;
    }

    bool anyOk = false;
    for (int i = 0; i < NUM_VEHICLES; i++) {
        JsonObject o = doc[i];
        String bat = o["bat"] | "", in = o["in"] | "", out = o["out"] | "";
        String lockState = o["lock"] | "", state = o["state"] | "", pres = o["pres"] | "";

        TeslaVehicleData v;
        v.name = VEHICLE_NAMES[i];
        v.batteryPercent = usable(bat) ? static_cast<int>(bat.toFloat()) : 0;
        v.insideTempF = usable(in) ? in.toFloat() : 0;
        v.outsideTempF = usable(out) ? out.toFloat() : 0;
        v.lockState = usable(lockState) ? lockState : "";
        v.state = usable(state) ? state : "";
        // binary_sensor presence reports "on"; a device_tracker reports "home".
        v.present = (pres == "on" || pres == "home");
        v.valid = usable(bat) || usable(lockState) || usable(state) || usable(pres);
        if (!v.valid) {
            continue;
        }

        DataLock lock;
        current[i] = v;
        anyOk = true;
    }
    return anyOk;
}

TeslaVehicleData TeslaManager::data(int index) const {
    DataLock lock;
    if (index < 0 || index >= NUM_VEHICLES) {
        return TeslaVehicleData();
    }
    return current[index];
}
