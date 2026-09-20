#include "weather_icon.h"

namespace {
void drawCloudBase(TFT_eSPI &tft, int cx, int cy, int s, int dy, uint16_t color) {
    tft.fillCircle(cx - 6 * s, cy - 4 * s + dy, 7 * s, color);
    tft.fillCircle(cx + 4 * s, cy - 7 * s + dy, 8 * s, color);
    tft.fillCircle(cx + 12 * s, cy - 4 * s + dy, 6 * s, color);
    tft.fillRoundRect(cx - 8 * s, cy - 4 * s + dy, 26 * s, 8 * s, 4 * s, color);
}
}  // namespace

void drawWeatherIcon(TFT_eSPI &tft, WeatherIcon icon, int cx, int cy, int scale, int monochromeOverride) {
    int s = scale;
    bool mono = monochromeOverride >= 0;
    uint16_t mainColor = mono ? static_cast<uint16_t>(monochromeOverride) : TFT_YELLOW;
    uint16_t cloudColor = mono ? static_cast<uint16_t>(monochromeOverride) : TFT_LIGHTGREY;
    uint16_t rainColor = mono ? static_cast<uint16_t>(monochromeOverride) : TFT_CYAN;

    switch (icon) {
        case WeatherIcon::SUN: {
            tft.fillCircle(cx, cy, 9 * s, mainColor);
            for (int a = 0; a < 360; a += 45) {
                float rad = a * PI / 180.0f;
                int x0 = cx + static_cast<int>(cosf(rad) * 12 * s);
                int y0 = cy + static_cast<int>(sinf(rad) * 12 * s);
                int x1 = cx + static_cast<int>(cosf(rad) * 16 * s);
                int y1 = cy + static_cast<int>(sinf(rad) * 16 * s);
                tft.drawLine(x0, y0, x1, y1, mainColor);
            }
            break;
        }
        case WeatherIcon::PARTLY_CLOUDY:
            tft.fillCircle(cx - 6 * s, cy - 4 * s, 7 * s, mainColor);
            tft.fillCircle(cx + 2 * s, cy + 2 * s, 8 * s, cloudColor);
            tft.fillCircle(cx + 10 * s, cy + 2 * s, 6 * s, cloudColor);
            tft.fillRoundRect(cx - 6 * s, cy + 2 * s, 22 * s, 9 * s, 4 * s, cloudColor);
            break;
        case WeatherIcon::CLOUDY:
        case WeatherIcon::FOG:
            tft.fillCircle(cx - 6 * s, cy, 8 * s, cloudColor);
            tft.fillCircle(cx + 4 * s, cy - 4 * s, 9 * s, cloudColor);
            tft.fillCircle(cx + 12 * s, cy, 7 * s, cloudColor);
            tft.fillRoundRect(cx - 10 * s, cy, 28 * s, 9 * s, 4 * s, cloudColor);
            break;
        case WeatherIcon::RAIN:
            drawCloudBase(tft, cx, cy, s, 0, cloudColor);
            for (int i = 0; i < 3; i++) {
                int x = cx - 4 * s + i * 7 * s;
                tft.drawLine(x, cy + 8 * s, x - 2 * s, cy + 15 * s, rainColor);
            }
            break;
        case WeatherIcon::SNOW:
            drawCloudBase(tft, cx, cy, s, 0, cloudColor);
            for (int i = 0; i < 3; i++) {
                int x = cx - 4 * s + i * 7 * s;
                tft.setTextColor(mono ? mainColor : TFT_WHITE);
                tft.drawString("*", x - 3 * s, cy + 6 * s, 2);
            }
            break;
        case WeatherIcon::STORM: {
            drawCloudBase(tft, cx, cy, s, 0, cloudColor);
            int bx = cx + 2 * s;
            int by = cy + 6 * s;
            tft.fillTriangle(bx, by, bx + 6 * s, by, bx, by + 8 * s, mainColor);
            tft.fillTriangle(bx, by + 8 * s, bx + 6 * s, by, bx + 6 * s, by + 8 * s, mainColor);
            break;
        }
    }
}
