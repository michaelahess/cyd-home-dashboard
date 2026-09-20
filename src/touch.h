#pragma once

#include <TFT_eSPI.h>

#include <cstdint>

struct TouchEvent {
    bool tapped = false;
    int x = 0;  // approximate screen x, 0..319
    int y = 0;  // approximate screen y, 0..239
};

// Wraps the XPT2046 touch controller, which on the CYD sits on its own VSPI
// bus (MOSI=32, MISO=39, SCLK=25, CS=33, IRQ=36) separate from the display's
// SPI bus, and turns a quick tap into an approximate screen (x, y).
//
// Resistive touch panels vary enough unit-to-unit that a single hardcoded
// calibration doesn't fit every board. Calibration bounds are therefore
// loaded from NVS (flash) rather than compiled in, so one firmware image
// works on any physical unit -- see calibrate() and main.cpp's first-boot
// flow.
class TouchTap {
public:
    // flipped: true when the display is running rotated 180 degrees from
    // its normal orientation (see the boot-button rotation toggle in
    // main.cpp). The touch panel's own wiring never changes, so screen
    // coordinates need to be mirrored on both axes to match.
    void begin(bool flipped);

    // True once calibration bounds have been loaded (from NVS, or from a
    // calibrate() call this session). False means poll() will still work
    // but with the uncalibrated 0..4095 raw range, which is unlikely to
    // line up with the screen.
    bool isCalibrated() const;

    // Runs an on-screen 5-point calibration (draws targets on tft, blocks
    // waiting for each tap), then saves the computed bounds to NVS and
    // applies them immediately. Safe to call any time after begin(),
    // including from a running app (e.g. a Settings page's "Recalibrate"
    // action), since it reuses this object's own touch controller instance
    // rather than opening a second one on the same SPI bus.
    void calibrate(TFT_eSPI &tft);

    // Call every loop iteration. Returns a tapped=true event once per
    // completed tap (press + release within a short time, no drag),
    // tapped=false otherwise.
    TouchEvent poll();

private:
    bool flipped = false;
    bool calibrated = false;
    int rawXMin = 0, rawXMax = 4095;
    int rawYMin = 0, rawYMax = 4095;

    bool wasTouched = false;
    uint32_t touchDownMs = 0;
    int downRawX = 0;
    int downRawY = 0;

    void loadCalibration();
    void saveCalibration();
};
