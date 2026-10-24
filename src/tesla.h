#pragma once

#include <Arduino.h>

struct TeslaVehicleData {
    bool valid = false;
    String name;
    int batteryPercent = 0;
    float insideTempF = 0;
    float outsideTempF = 0;
    String lockState;  // "locked" / "unlocked"
    String state;      // "online" / "offline" / "asleep"
    bool present = false;
};

// Per-vehicle telemetry (battery %, inside/outside temp, lock, online
// state, at-home presence) read from Home Assistant entities -- see the
// TESLA_* macros in secrets.h.
class TeslaManager {
public:
    static constexpr int NUM_VEHICLES = 2;

    void loop();

    // index 0/1 match VEHICLE_NAMES / the TESLA_* entity lists in secrets.h
    TeslaVehicleData data(int index) const;

private:
    TeslaVehicleData current[NUM_VEHICLES];
    unsigned long lastFetchMs = 0;

    bool fetchNow();
};
