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

// Per-zone thermostat status and control via Home Assistant `climate`
// entities (HVAC_ZONE_ENTITIES in secrets.h). HA's mode names are translated
// to the strings below (heat_cool -> "auto").
class HvacZonesManager {
public:
    static constexpr int NUM_ZONES = 5;

    void loop();

    HvacZoneData data(int index) const;

    // True if any zone's operatingState is actually "heating" or "cooling"
    // right now (not just configured for it).
    bool anyActivelyHeatingOrCooling() const;

    // command is one of: "off", "heat", "cool", "auto". Updates the cached mode immediately
    // (optimistic) rather than waiting for the next poll.
    bool sendMode(int index, const char *command);

    // Adjusts the setpoint relevant to the zone's current mode (heating or
    // cooling) by deltaF degrees, clamped to a sane 60-85F range.
    bool adjustSetpoint(int index, float deltaF);

    // Cycles through the fan modes the climate entity reports (fan_modes).
    bool cycleFanMode(int index);

private:
    HvacZoneData current[NUM_ZONES];
    // Comma-separated climate `fan_modes` / `hvac_modes` per zone, so fan
    // cycling and the "auto" button can use what each entity supports.
    String fanModesCsv[NUM_ZONES];
    String hvacModesCsv[NUM_ZONES];
    unsigned long lastFetchMs = 0;

    bool fetchNow();
};
