#include "tank_temps.h"

#include <WiFi.h>

#include "data_mutex.h"
#include "ha_client.h"
#include "secrets.h"

namespace {
constexpr unsigned long FETCH_INTERVAL_MS = 30UL * 1000UL;
// Which Home Assistant sensors to read, and what to call them, are secrets.h
// variables so they stay out of version control -- see TANK_ENTITY_IDS /
// TANK_LABELS in secrets.h.example (NUM_TANKS in tank_temps.h sizes both).
constexpr const char *TANK_ENTITIES[TankManager::NUM_TANKS] = TANK_ENTITY_IDS;
constexpr const char *TANK_NAMES[TankManager::NUM_TANKS] = TANK_LABELS;
}  // namespace

void TankManager::loop() {
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

bool TankManager::fetchNow() {
    // One POST returning every tank as {"t0": <degF>, "t1": ...}. The -999
    // sentinel distinguishes a genuinely unavailable/unknown HA state (which
    // the `float()` filter would otherwise silently turn into 0, easily
    // mistaken for a real near-freezing reading) from a real temperature.
    String tpl = "{";
    for (int i = 0; i < NUM_TANKS; i++) {
        tpl += "\"t" + String(i) + "\": {{ states('" + TANK_ENTITIES[i] + "') | float(-999) }}";
        if (i < NUM_TANKS - 1) {
            tpl += ", ";
        }
    }
    tpl += "}";
    JsonDocument doc;
    bool ok = haFetchTemplate(tpl, doc);

    DataLock lock;
    for (int i = 0; i < NUM_TANKS; i++) {
        current[i].label = TANK_NAMES[i];
        if (ok) {
            float v = doc["t" + String(i)] | -999.0f;
            current[i].valid = (v > -900.0f);
            current[i].tempF = v;
        }
    }
    return ok;
}

TankData TankManager::data(int index) const {
    DataLock lock;
    if (index < 0 || index >= NUM_TANKS) {
        return TankData();
    }
    return current[index];
}
