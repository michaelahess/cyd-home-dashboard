#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "../unifi_camera.h"
#include "page.h"

// Live-ish UniFi Protect snapshot (refreshed every ~1.5 s while this page is
// visible). Tap the middle of the screen to cycle cameras when more than one
// is configured; top/bottom edge strips still change pages as usual.
class CameraPage : public Page {
public:
    void begin(TFT_eSPI &tftRef, UnifiCameraManager &camRef);

    void onShow() override;
    void loop() override;
    void onTap(int x, int y) override;

private:
    TFT_eSPI *tft = nullptr;
    UnifiCameraManager *cam = nullptr;
    int camIndex = 0;
    uint32_t lastSeq = 0;
    uint8_t *jpeg = nullptr;
    size_t jpegLen = 0;
    String lastStatusDrawn;
    bool gotFrame = false;

    void drawStatus(const String &s);
    void drawFrame();
};
