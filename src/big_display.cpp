#include "big_display.h"

#if defined(CYD_BIG)

#include <Arduino_GFX_Library.h>

namespace {
// QSPI wiring from the maker's demo (ESP32_4827A043_QSPI config); backlight
// is GPIO 1 and is handled in main.cpp.
constexpr int PIN_CS = 45;
constexpr int PIN_SCK = 47;
constexpr int PIN_D0 = 21;
constexpr int PIN_D1 = 48;
constexpr int PIN_D2 = 40;
constexpr int PIN_D3 = 39;

// The UI only changes about once a second, so ~12 fps is plenty and keeps
// the QSPI bus and CPU mostly free.
constexpr uint32_t PRESENT_INTERVAL_MS = 80;

Arduino_DataBus *bus = nullptr;
Arduino_G *panel = nullptr;
Arduino_Canvas *cv = nullptr;
uint32_t lastPresentMs = 0;
}  // namespace

namespace BigDisplay {

void begin(bool flipped) {
    bus = new Arduino_ESP32QSPI(PIN_CS, PIN_SCK, PIN_D0, PIN_D1, PIN_D2, PIN_D3);
    panel = new Arduino_NV3041A(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */);
    cv = new Arduino_Canvas(PANEL_W, PANEL_H, panel);
    cv->begin();
    // Only 0 and 180 degrees are reported to work on this panel.
    cv->setRotation(flipped ? 2 : 0);
    cv->fillScreen(0x0000);
    cv->flush();
}

Arduino_GFX *canvas() {
    return cv;
}

void flush() {
    if (cv != nullptr) {
        cv->flush();
    }
}

void clear() {
    if (cv == nullptr) {
        return;
    }
    cv->fillScreen(0x0000);
    cv->flush();
}

void present(const void *ui) {
    uint32_t now = millis();
    if (cv == nullptr || now - lastPresentMs < PRESENT_INTERVAL_MS) {
        return;
    }
    lastPresentMs = now;
    // TFT_eSprite stores 16-bit pixels byte-swapped (big-endian), which is
    // what the Be variant expects.
    cv->draw16bitBeRGBBitmap(UI_X, UI_Y, const_cast<uint16_t *>(static_cast<const uint16_t *>(ui)), UI_W, UI_H);
    cv->flush();
}

}  // namespace BigDisplay

#endif
