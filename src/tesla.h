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

// Real per-vehicle telemetry via Hubitat's TeslaMate-driven devices --
// replaces the earlier Wall-Connector-only placeholder now that this data
// is confirmed available (Tesla battery %, inside/outside temp, lock,
// online state, at-home presence).
class TeslaManager {
public:
    static constexpr int NUM_VEHICLES = 2;

    void loop();

    // index 0 = Tessi, 1 = Cinder
    TeslaVehicleData data(int index) const;

private:
    TeslaVehicleData current[NUM_VEHICLES];
    unsigned long lastFetchMs = 0;

    bool fetchNow();
};
