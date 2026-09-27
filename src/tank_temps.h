#pragma once

#include <Arduino.h>

struct TankData {
    const char *label = "";
    float tempF = 0;
    bool valid = false;
};

// Polls Home Assistant for each tank's probe temperature via
// /api/template -- one small POST returning all of them in a single
// response, same pattern as every other HA-backed data source in this
// project. Which tanks/entities are read, and their display names, come from
// TANK_ENTITY_IDS/TANK_LABELS in secrets.h (see secrets.h.example).
class TankManager {
public:
    static constexpr int NUM_TANKS = 7;

    void loop();

    // Returns a locked copy -- safe to call from the render loop while the
    // background task may be mid-write to `current`.
    TankData data(int index) const;

private:
    TankData current[NUM_TANKS];
    unsigned long lastFetchMs = 0;

    bool fetchNow();
};
