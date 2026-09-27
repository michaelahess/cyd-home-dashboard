#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "page.h"
#include "tank_temps.h"

// Shows a slice of TankManager's tanks (one instance per page, since 7
// tanks don't comfortably fit on one screen) -- which slice is set via
// begin()'s startIndex/count.
class TankPage : public Page {
public:
    void begin(TFT_eSPI &tftRef, const TankManager &tanksRef, int startIndex, int count);

    void onShow() override;
    void loop() override;

private:
    TFT_eSPI *tft = nullptr;
    const TankManager *tanks = nullptr;
    int startIndex = 0;
    int count = 0;
    uint32_t lastDrawMs = 0;
    bool hasLastDrawn = false;
    TankData lastDrawn[TankManager::NUM_TANKS];

    void drawRow(int rowIndex, const TankData &t);
    void drawAll();
};
