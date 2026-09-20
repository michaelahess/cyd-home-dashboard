#pragma once

#include <TFT_eSPI.h>

#include "beep.h"
#include "page.h"
#include "touch.h"

// On-device settings that used to require editing a build flag/constant
// and reflashing: touch recalibration, display rotation, night-mode
// dimming level, and beep volume.
class SettingsPage : public Page {
public:
    void begin(TFT_eSPI &tftRef, TouchTap &touchRef, Beeper &beeperRef);

    void onShow() override;
    void loop() override;
    void onTap(int x, int y) override;

private:
    TFT_eSPI *tft = nullptr;
    TouchTap *touch = nullptr;
    Beeper *beeper = nullptr;

    void draw();
};
