#pragma once

#include <Arduino.h>

struct HvacZoneData {
    bool valid = false;
    String label;
    String mode;  // "off" / "heat" / "cool" / "auto" / "dry" / "emergency heat"
    float currentTempF = 0;
    float coolingSetpointF = 0;
    float heatingSetpointF = 0;
    String fanMode;
    // "heating" / "cooling" / "idle" / "fan only" / "pending heat" / etc --
    // whether the unit is *actually* running right now, which `mode` alone
    // doesn't tell you (mode could be "auto" while idle because the
    // setpoint is already satisfied).
    String operatingState;
};

// Real per-zone thermostat status and control via Hubitat's "Mitsubishi
// Heat Pump MQTT" driver -- replaces the earlier single
// aggregate-power-only reading now that per-zone mode/temp/setpoint data
// (and real, well-documented Thermostat-capability commands) are confirmed
// available.
class HvacZonesManager {
public:
    static constexpr int NUM_ZONES = 5;

    void loop();

    HvacZoneData data(int index) const;

    // True if any zone's operatingState is actually "heating" or "cooling"
    // right now (not just configured for it).
    bool anyActivelyHeatingOrCooling() const;

    // command is one of: "off", "heat", "cool", "auto" (matches Hubitat's
    // no-argument mode commands). Updates the cached mode immediately
    // (optimistic) rather than waiting for the next poll.
    bool sendMode(int index, const char *command);

    // Adjusts the setpoint relevant to the zone's current mode (heating or
    // cooling) by deltaF degrees, clamped to a sane 60-85F range.
    bool adjustSetpoint(int index, float deltaF);

    // Cycles fan mode: auto -> on -> circulate -> auto.
    bool cycleFanMode(int index);

private:
    HvacZoneData current[NUM_ZONES];
    unsigned long lastFetchMs = 0;

    bool fetchNow();
};
