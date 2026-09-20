#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "home_power.h"
#include "page.h"

// Full solar/grid/battery dashboard: current + daily production, battery
// level, house load, grid import/export, and battery charge/discharge --
// all from the same HomePowerManager fetch the clock page's status row uses.
class SolarPage : public Page {
public:
    void begin(TFT_eSPI &tftRef, const HomePowerManager &powerRef);

    void onShow() override;
    void loop() override;

private:
    TFT_eSPI *tft = nullptr;
    const HomePowerManager *power = nullptr;
    uint32_t lastDrawMs = 0;
    HomePowerData lastDrawn;
    bool hasLastDrawn = false;

    void drawTile(int x, int y, int w, int h, const char *label, const String &value, uint16_t color);
    void drawAll();
    static bool sameValues(const HomePowerData &a, const HomePowerData &b);
};
