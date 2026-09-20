#pragma once

#include <TFT_eSPI.h>

#include "weather.h"

// Draws a simple hand-drawn condition icon (no bitmap assets), centered at
// (cx, cy). scale 1 matches the forecast rows' size; scale 2 is roughly
// double, used for the clock/weather page's larger icon.
//
// monochromeOverride: if >= 0, every stroke of the icon uses this single
// color instead of its normal per-shape palette (sun=yellow, cloud=grey,
// rain=cyan, etc.) -- used for night mode, where the ask was for the icon
// to read as "the same shade of red as everything else", not multicolored.
void drawWeatherIcon(TFT_eSPI &tft, WeatherIcon icon, int cx, int cy, int scale, int monochromeOverride = -1);
