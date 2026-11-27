#pragma once

#if defined(CYD_BIG)
#include <Arduino.h>

class Arduino_GFX;

// "Mike Solar Big" (Guition JC4827W543C) display glue. The panel is a 480x272
// NV3041A on a QSPI bus, driven through Arduino_GFX with a PSRAM canvas. The
// existing pages draw a 320x240 UI into an off-screen TFT_eSprite; present()
// copies that centered into the panel. The Camera page draws full-screen
// straight into canvas() instead.
namespace BigDisplay {
constexpr int PANEL_W = 480;
constexpr int PANEL_H = 272;
constexpr int UI_W = 320;
constexpr int UI_H = 240;
constexpr int UI_X = (PANEL_W - UI_W) / 2;
constexpr int UI_Y = (PANEL_H - UI_H) / 2;

void begin(bool flipped);
Arduino_GFX *canvas();
// Copy the 320x240 big-endian RGB565 UI buffer (a TFT_eSprite's pointer)
// centered into the panel. Rate limited; safe to call every loop. TFT_eSPI is
// deliberately NOT included here: its macros clash with Arduino_GFX.
void present(const void *ui320x240);
void clear();                   // black the whole panel now
void flush();                   // push the canvas to the panel
}  // namespace BigDisplay
#endif
