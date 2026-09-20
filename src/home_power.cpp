#include "home_power.h"

#include <WiFi.h>

#include "data_mutex.h"
#include "ha_client.h"

namespace {
constexpr unsigned long FETCH_INTERVAL_MS = 30UL * 1000UL;
}  // namespace

void HomePowerManager::loop() {
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

bool HomePowerManager::fetchNow() {
    // Entity IDs confirmed live via a direct /api/states query -- the
    // lux_*/eg4_* names used by the old Homepage dashboard config are
    // stale and always read 0.
    JsonDocument doc;
    bool ok = haFetchTemplate(
        "{"
        "\"battery\": {{ states('sensor.battery_bank_44200e0218_battery_bank_capacity_percent') | float(0) }}, "
        "\"load\": {{ states('sensor.flexboss21_44200e0218_total_load_power') | float(0) }}, "
        "\"solar\": {{ states('sensor.flexboss21_44200e0218_pv_total_power') | float(0) }}, "
        "\"yield\": {{ states('sensor.flexboss21_44200e0218_yield') | float(0) }}, "
        "\"grid\": {{ states('sensor.flexboss21_44200e0218_grid_power') | float(0) }}, "
        "\"battpower\": {{ states('sensor.flexboss21_44200e0218_battery_power') | float(0) }}, "
        "\"battstatus\": \"{{ states('sensor.flexboss21_44200e0218_battery_status') }}\""
        "}",
        doc);
    if (!ok) {
        return false;
    }

    DataLock lock;
    current.batteryPercent = static_cast<int>(lroundf(doc["battery"] | 0.0f));
    current.loadWatts = static_cast<int>(lroundf(doc["load"] | 0.0f));
    current.solarWatts = static_cast<int>(lroundf(doc["solar"] | 0.0f));
    current.dailyYieldKwh = doc["yield"] | 0.0f;
    current.gridWatts = static_cast<int>(lroundf(doc["grid"] | 0.0f));
    current.batteryPowerWatts = static_cast<int>(lroundf(doc["battpower"] | 0.0f));
    current.batteryStatus = doc["battstatus"] | "";
    current.valid = true;
    return true;
}

HomePowerData HomePowerManager::data() const {
    DataLock lock;
    return current;
}
