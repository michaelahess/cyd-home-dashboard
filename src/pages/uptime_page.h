#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "page.h"
#include "uptime_kuma.h"

// Simple up/down summary of the family's Uptime Kuma monitors: a happy
// checkmark when everything's up, otherwise a count and the names of
// whatever's down.
class UptimePage : public Page {
public:
    void begin(TFT_eSPI &tftRef, const UptimeKumaManager &kumaRef);

    void onShow() override;
    void loop() override;

private:
    TFT_eSPI *tft = nullptr;
    const UptimeKumaManager *kuma = nullptr;
    uint32_t lastDrawMs = 0;
    bool hasLastDrawn = false;
    bool lastConfigLoaded = false;
    int lastUp = -1;
    int lastDown = -1;
    String lastDownList;

    void drawAll();
};
