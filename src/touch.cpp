#include "touch.h"

#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

namespace {
constexpr int TOUCH_CS = 33;
constexpr int TOUCH_IRQ = 36;
constexpr int TOUCH_MOSI = 32;
constexpr int TOUCH_MISO = 39;
constexpr int TOUCH_SCLK = 25;

// Maximum press-to-release duration to count as a tap rather than a
// press-and-hold.
constexpr uint32_t MAX_TAP_MS = 600;

int mapRawToScreen(int raw, int rawMin, int rawMax, int outMax) {
    long v = map(raw, rawMin, rawMax, 0, outMax);
    return constrain(static_cast<int>(v), 0, outMax);
}

struct CalPoint {
    int sx, sy;
    const char *label;
};

constexpr CalPoint POINTS[5] = {
    {20, 20, "1: top-left"},
    {300, 20, "2: top-right"},
    {160, 120, "3: center"},
    {20, 220, "4: bottom-left"},
    {300, 220, "5: bottom-right"},
};

XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
}  // namespace

void TouchTap::begin(bool flippedIn) {
    flipped = flippedIn;
    // TFT_eSPI drives the display bus directly at the register level, not
    // through the Arduino global SPI object -- so it's free to repurpose
    // here for the touch controller's separate bus. XPT2046_Touchscreen's
    // own begin() calls SPI.begin() with no args internally, which on ESP32
    // no-ops once already started, so this explicit pin mapping sticks.
    SPI.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    ts.begin();
    loadCalibration();
}

bool TouchTap::isCalibrated() const {
    return calibrated;
}

void TouchTap::loadCalibration() {
    Preferences prefs;
    prefs.begin("touch", true);
    calibrated = prefs.getBool("cal", false);
    if (calibrated) {
        rawXMin = prefs.getInt("xmin", rawXMin);
        rawXMax = prefs.getInt("xmax", rawXMax);
        rawYMin = prefs.getInt("ymin", rawYMin);
        rawYMax = prefs.getInt("ymax", rawYMax);
    }
    prefs.end();
}

void TouchTap::saveCalibration() {
    Preferences prefs;
    prefs.begin("touch", false);
    prefs.putBool("cal", true);
    prefs.putInt("xmin", rawXMin);
    prefs.putInt("xmax", rawXMax);
    prefs.putInt("ymin", rawYMin);
    prefs.putInt("ymax", rawYMax);
    prefs.end();
    calibrated = true;
}

void TouchTap::calibrate(TFT_eSPI &tft) {
    int rawX[5];
    int rawY[5];

    for (int i = 0; i < 5; i++) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(String("Tap point ") + POINTS[i].label, 160, 120, 4);

        tft.drawFastHLine(POINTS[i].sx - 12, POINTS[i].sy, 24, TFT_RED);
        tft.drawFastVLine(POINTS[i].sx, POINTS[i].sy - 12, 24, TFT_RED);
        tft.drawCircle(POINTS[i].sx, POINTS[i].sy, 8, TFT_RED);

        // Blocking on purpose -- calibration is a modal flow (first boot,
        // or an explicit "Recalibrate" action), never running alongside
        // normal page rendering.
        while (!ts.touched()) {
            delay(10);
        }
        TS_Point p = ts.getPoint();
        rawX[i] = p.x;
        rawY[i] = p.y;

        while (ts.touched()) {
            delay(10);
        }
        delay(300);  // debounce before showing the next target
    }

    // Corners only (indices 0,1,3,4) -- the center point (2) is a sanity
    // check for the person calibrating, not used in the math. Averaging the
    // two points on each edge cancels a little of the per-tap jitter
    // inherent to resistive panels.
    rawXMin = (rawX[0] + rawX[3]) / 2;  // top-left + bottom-left
    rawXMax = (rawX[1] + rawX[4]) / 2;  // top-right + bottom-right
    rawYMin = (rawY[0] + rawY[1]) / 2;  // top-left + top-right
    rawYMax = (rawY[3] + rawY[4]) / 2;  // bottom-left + bottom-right
    saveCalibration();

    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Calibration saved!", 160, 120, 4);
    tft.setTextDatum(TL_DATUM);
    delay(800);

    // Clear any latent press so the caller doesn't see a phantom tap the
    // instant it resumes polling.
    wasTouched = false;
}

TouchEvent TouchTap::poll() {
    TouchEvent ev;
    bool touched = ts.touched();

    if (touched && !wasTouched) {
        TS_Point p = ts.getPoint();
        downRawX = p.x;
        downRawY = p.y;
        touchDownMs = millis();
    } else if (!touched && wasTouched) {
        uint32_t heldMs = millis() - touchDownMs;
        wasTouched = touched;
        if (heldMs <= MAX_TAP_MS) {
            int screenX = mapRawToScreen(downRawX, rawXMin, rawXMax, 319);
            int screenY = mapRawToScreen(downRawY, rawYMin, rawYMax, 239);
            if (flipped) {
                // Rotating the display 180 degrees doesn't move the touch
                // panel, so mirror both axes to match what's now on screen.
                screenX = 319 - screenX;
                screenY = 239 - screenY;
            }
            ev.tapped = true;
            ev.x = screenX;
            ev.y = screenY;
        }
        return ev;
    }

    wasTouched = touched;
    return ev;
}
