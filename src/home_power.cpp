#include "home_power.h"

#include <WiFi.h>

#include "data_mutex.h"
#include "ha_client.h"
#include "secrets.h"

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
    // Which Home Assistant entities to read are secrets.h variables -- see
    // POWER_* in secrets.h.example.
    String tpl = "{";
    tpl += "\"battery\": {{ states('" + String(POWER_BATTERY_SOC) + "') | float(0) }}, ";
    tpl += "\"load\": {{ states('" + String(POWER_LOAD_W) + "') | float(0) }}, ";
    tpl += "\"solar\": {{ states('" + String(POWER_SOLAR_W) + "') | float(0) }}, ";
    tpl += "\"yield\": {{ states('" + String(POWER_YIELD_TODAY_KWH) + "') | float(0) }}, ";
    tpl += "\"grid\": {{ states('" + String(POWER_GRID_W) + "') | float(0) }}, ";
    tpl += "\"battpower\": {{ states('" + String(POWER_BATTERY_W) + "') | float(0) }}, ";
    tpl += "\"battstatus\": \"{{ states('" + String(POWER_BATTERY_STATUS) + "') }}\"";
    tpl += "}";
    JsonDocument doc;
    bool ok = haFetchTemplate(tpl, doc);
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
