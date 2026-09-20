#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "page.h"
#include "tesla.h"

// Both vehicles' battery %, inside/outside temp, lock state, and online
// status via Hubitat's TeslaMate-driven devices.
class TeslaPage : public Page {
public:
    void begin(TFT_eSPI &tftRef, const TeslaManager &teslaRef);

    void onShow() override;
    void loop() override;

private:
    TFT_eSPI *tft = nullptr;
    const TeslaManager *tesla = nullptr;
    uint32_t lastDrawMs = 0;
    TeslaVehicleData lastDrawn[TeslaManager::NUM_VEHICLES];
    bool hasLastDrawn = false;

    void drawVehicle(int y, const TeslaVehicleData &v);
    void drawAll();
    static bool sameValues(const TeslaVehicleData &a, const TeslaVehicleData &b);
};
