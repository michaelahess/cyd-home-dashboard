#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "fish_pump.h"
#include "page.h"

// Auto-top-off pump status: today's run count, last/next run time, and
// wet/dry reservoir status.
class PumpPage : public Page {
public:
    void begin(TFT_eSPI &tftRef, const FishPumpManager &pumpRef);

    void onShow() override;
    void loop() override;

private:
    TFT_eSPI *tft = nullptr;
    const FishPumpManager *pump = nullptr;
    uint32_t lastDrawMs = 0;
    FishPumpData lastDrawn;
    bool hasLastDrawn = false;

    void drawAll();
    static bool sameValues(const FishPumpData &a, const FishPumpData &b);
};
