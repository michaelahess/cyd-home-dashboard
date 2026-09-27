#include "fish_pump.h"

#include <WiFi.h>

#include "data_mutex.h"
#include "ha_client.h"
#include "secrets.h"

namespace {
constexpr unsigned long FETCH_INTERVAL_MS = 30UL * 1000UL;
}  // namespace

void FishPumpManager::loop() {
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

bool FishPumpManager::fetchNow() {
    // Which Home Assistant entities describe the pump (run counter, last-run
    // datetime, hourly timer, reservoir wet/dry sensor) are secrets.h
    // variables -- see PUMP_* in secrets.h.example.
    const String timer = PUMP_TIMER;
    const String lastRun = PUMP_LAST_RUN;
    String tpl = "{";
    tpl += "\"runs_today\": {{ states('" + String(PUMP_RUNS_COUNTER) + "') | int(0) }}, ";
    tpl += "\"last_run_hour\": {{ state_attr('" + lastRun + "', 'hour') | int(0) }}, ";
    tpl += "\"last_run_minute\": {{ state_attr('" + lastRun + "', 'minute') | int(0) }}, ";
    tpl += "\"next_run_active\": {{ 'true' if is_state('" + timer + "', 'active') else 'false' }}, ";
    tpl += "\"next_run_epoch\": {{ (as_timestamp(state_attr('" + timer + "', 'finishes_at')) | int(0)) ";
    tpl += "if is_state('" + timer + "', 'active') else 0 }}, ";
    tpl += "\"wet\": {{ 'true' if is_state('" + String(PUMP_WET_SENSOR) + "', 'on') else 'false' }}";
    tpl += "}";
    JsonDocument doc;
    bool ok = haFetchTemplate(tpl, doc);
    if (!ok) {
        return false;
    }

    DataLock lock;
    current.runsToday = doc["runs_today"] | 0;
    current.lastRunHour = doc["last_run_hour"] | 0;
    current.lastRunMinute = doc["last_run_minute"] | 0;
    current.nextRunActive = doc["next_run_active"] | false;
    current.nextRunEpoch = doc["next_run_epoch"] | 0L;
    current.wet = doc["wet"] | false;
    current.valid = true;
    return true;
}

FishPumpData FishPumpManager::data() const {
    DataLock lock;
    return current;
}
