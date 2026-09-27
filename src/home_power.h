#pragma once

#include <Arduino.h>

struct HomePowerData {
    bool valid = false;
    int batteryPercent = 0;
    int loadWatts = 0;

    // Solar/grid detail, for the dedicated solar page -- all from the same
    // EG4 FlexBoss21 integration as battery/load above, fetched together in
    // one request rather than polling the inverter twice.
    int solarWatts = 0;         // current PV production
    float dailyYieldKwh = 0;    // today's production so far
    int gridWatts = 0;          // + = importing from grid, - = exporting
    int batteryPowerWatts = 0;  // + = charging, - = discharging
    String batteryStatus;       // e.g. "Charging" / "Idle"
};

// Polls the house EG4 battery/inverter system via Home Assistant's
// /api/template endpoint -- one small POST returning several sensor values
// in a single response, no direct Modbus/serial-bridge access.
class HomePowerManager {
public:
    void loop();

    // Returns a locked copy -- safe to call from the render loop while the
    // background task may be mid-write to `current`.
    HomePowerData data() const;

private:
    HomePowerData current;
    unsigned long lastFetchMs = 0;

    bool fetchNow();
};
