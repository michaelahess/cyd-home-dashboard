#pragma once

#include <Arduino.h>

struct FishPumpData {
    bool valid = false;
    int runsToday = 0;
    int lastRunHour = 0;    // 24h, local time (already local as reported by HA)
    int lastRunMinute = 0;
    bool nextRunActive = false;  // false if the hourly timer isn't currently running
    long nextRunEpoch = 0;       // unix timestamp (UTC), only meaningful if nextRunActive
    bool wet = false;            // fluid-input sensor: true = wet (has fluid), false = dry
};

// Polls a Home Assistant-integrated auto-top-off pump (Apollo Automation
// dosing pump on an hourly timer) via HA's /api/template endpoint -- one
// small POST returning several values in a single response, same pattern
// as every other HA-backed data source in this project.
class FishPumpManager {
public:
    void loop();

    // Returns a locked copy -- safe to call from the render loop while the
    // background task may be mid-write to `current`.
    FishPumpData data() const;

private:
    FishPumpData current;
    unsigned long lastFetchMs = 0;

    bool fetchNow();
};
