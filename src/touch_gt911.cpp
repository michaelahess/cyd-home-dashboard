// GT911 capacitive touch for the "Mike Solar Big" board (CYD_BIG). Same
// TouchTap interface as the resistive XPT2046 version in touch.cpp, so the
// rest of the code doesn't change. Pins from the maker's demo repo:
// SDA 8, SCL 4, RST 38, INT 3. UNTESTED on hardware as of this commit.
#if defined(CYD_BIG)

#include <Arduino.h>
#include <Wire.h>

#include "big_display.h"
#include "display_settings.h"
#include "touch.h"

// If taps land mirrored/swapped on the real board, flip these (build flags).
// #define TOUCH_SWAP_XY
// #define TOUCH_FLIP_X
// #define TOUCH_FLIP_Y

namespace {
constexpr int SDA_PIN = 8;
constexpr int SCL_PIN = 4;
constexpr int RST_PIN = 38;
constexpr int INT_PIN = 3;

constexpr uint32_t MAX_TAP_MS = 600;
constexpr uint32_t HOLD_MS = 80;  // GT911 only flags *new* data; treat a short gap as "still down"

uint8_t addr = 0x5D;
bool present = false;
uint32_t lastReportMs = 0;

bool readRegs(uint16_t reg, uint8_t *buf, size_t n) {
    Wire.beginTransmission(addr);
    Wire.write(static_cast<uint8_t>(reg >> 8));
    Wire.write(static_cast<uint8_t>(reg & 0xFF));
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    if (Wire.requestFrom(static_cast<int>(addr), static_cast<int>(n)) != static_cast<int>(n)) {
        return false;
    }
    for (size_t i = 0; i < n; i++) {
        buf[i] = Wire.read();
    }
    return true;
}

void writeReg(uint16_t reg, uint8_t v) {
    Wire.beginTransmission(addr);
    Wire.write(static_cast<uint8_t>(reg >> 8));
    Wire.write(static_cast<uint8_t>(reg & 0xFF));
    Wire.write(v);
    Wire.endTransmission();
}
}  // namespace

void TouchTap::begin() {
    // Reset with INT held high selects address 0x5D.
    pinMode(INT_PIN, OUTPUT);
    pinMode(RST_PIN, OUTPUT);
    digitalWrite(INT_PIN, HIGH);
    digitalWrite(RST_PIN, LOW);
    delay(10);
    digitalWrite(RST_PIN, HIGH);
    delay(10);
    pinMode(INT_PIN, INPUT);
    delay(60);

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);
    for (uint8_t a : {static_cast<uint8_t>(0x5D), static_cast<uint8_t>(0x14)}) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
            addr = a;
            present = true;
            break;
        }
    }
    Serial.printf("GT911 touch %s (addr 0x%02X)\n", present ? "found" : "NOT FOUND", addr);
    calibrated = true;  // capacitive: no calibration step
}

bool TouchTap::isCalibrated() const {
    return calibrated;
}

void TouchTap::calibrate(TFT_eSPI & /*tft*/) {
    // Not needed for capacitive touch; the Settings page button is a no-op here.
}

TouchEvent TouchTap::poll() {
    TouchEvent ev;
    if (!present) {
        return ev;
    }
    uint32_t now = millis();

    bool touched = wasTouched && (now - lastReportMs) < HOLD_MS;
    uint8_t st = 0;
    int x = 0, y = 0;
    bool haveXY = false;
    if (readRegs(0x814E, &st, 1) && (st & 0x80)) {
        if ((st & 0x0F) > 0) {
            uint8_t p[4];
            if (readRegs(0x8150, p, 4)) {
                x = p[0] | (p[1] << 8);
                y = p[2] | (p[3] << 8);
                haveXY = true;
                touched = true;
                lastReportMs = now;
            }
        } else {
            touched = false;
        }
        writeReg(0x814E, 0);  // acknowledge so the next report can arrive
    }

    if (touched && !wasTouched) {
        wasTouched = true;
        touchDownMs = now;
        sumRawX = 0;
        sumRawY = 0;
        sampleCount = 0;
    }
    if (touched && haveXY) {
        sumRawX += x;
        sumRawY += y;
        sampleCount++;
    }
    if (!touched && wasTouched) {
        wasTouched = false;
        if (sampleCount > 0 && (now - touchDownMs) <= MAX_TAP_MS) {
            int px = static_cast<int>(sumRawX / sampleCount);
            int py = static_cast<int>(sumRawY / sampleCount);
#if defined(TOUCH_SWAP_XY)
            int t = px;
            px = py;
            py = t;
#endif
#if defined(TOUCH_FLIP_X)
            px = BigDisplay::PANEL_W - 1 - px;
#endif
#if defined(TOUCH_FLIP_Y)
            py = BigDisplay::PANEL_H - 1 - py;
#endif
            if (DisplaySettings::flipped()) {
                px = BigDisplay::PANEL_W - 1 - px;
                py = BigDisplay::PANEL_H - 1 - py;
            }
            // Panel coordinates -> the centered 320x240 UI area. Taps in the
            // black margin clamp to the edge (top margin = previous page).
            ev.tapped = true;
            ev.x = constrain(px - BigDisplay::UI_X, 0, BigDisplay::UI_W - 1);
            ev.y = constrain(py - BigDisplay::UI_Y, 0, BigDisplay::UI_H - 1);
        }
    }
    return ev;
}

void TouchTap::loadCalibration() {}
void TouchTap::saveCalibration() {}

#endif
