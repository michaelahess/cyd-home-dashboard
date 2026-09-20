#include "clock_weather_page.h"

#include "weather_icon.h"

namespace {
constexpr int WEATHER_Y0 = 177;
constexpr int WEATHER_H = 60;

// Battery color follows charge level -- the same green/yellow/red convention
// as a phone's battery icon, so the color itself carries meaning.
uint16_t batteryColor(TFT_eSPI &tft, int percent) {
    if (percent >= 50) {
        return tft.color565(0x0c, 0xa3, 0x0c);  // good / green
    }
    if (percent >= 20) {
        return tft.color565(0xfa, 0xb2, 0x19);  // warning / amber
    }
    return tft.color565(0xd0, 0x3b, 0x3b);  // critical / red
}

// A genuinely dim red rather than TFT_RED's full saturation, to match the
// dimmed backlight/LED at night (see main.cpp's night-mode handling).
uint16_t dimRed(TFT_eSPI &tft) {
    return tft.color565(0x70, 0x00, 0x00);
}

// Battery outline + terminal nub, filled proportionally to charge level.
void drawBatteryIcon(TFT_eSPI &tft, int x, int y, int percent, uint16_t fillColor) {
    constexpr int W = 22, H = 12, NUB = 3;
    tft.drawRoundRect(x, y, W, H, 2, TFT_WHITE);
    tft.fillRect(x + W, y + H / 2 - 2, NUB, 4, TFT_WHITE);
    int innerW = W - 4;
    int fillW = (innerW * constrain(percent, 0, 100)) / 100;
    tft.fillRect(x + 2, y + 2, innerW, H - 4, TFT_BLACK);
    if (fillW > 0) {
        tft.fillRect(x + 2, y + 2, fillW, H - 4, fillColor);
    }
}

// Small lightning bolt for house load, centered at (cx, cy).
void drawBoltIcon(TFT_eSPI &tft, int cx, int cy, uint16_t color) {
    tft.fillTriangle(cx, cy - 8, cx + 6, cy - 8, cx, cy, color);
    tft.fillTriangle(cx, cy, cx + 6, cy - 8, cx + 6, cy, color);
}
}  // namespace

void ClockWeatherPage::begin(TFT_eSPI &tftRef, const WeatherManager &weatherRef, const HomePowerManager &powerRef) {
    tft = &tftRef;
    weather = &weatherRef;
    power = &powerRef;
}

void ClockWeatherPage::onShow() {
    tft->fillScreen(TFT_BLACK);
    lastDrawnHour = -1;
    lastDrawnMinute = -1;
    lastDrawnYday = -1;
    lastWeatherValid = false;
    lastTempDrawn = -9999;
    lastWeatherCodeDrawn = -1;
    lastPowerValid = false;
    lastBatteryDrawn = -9999;
    lastLoadDrawn = -9999;
}

void ClockWeatherPage::drawPowerRow() {
    const HomePowerData &p = power->data();
    if (p.valid == lastPowerValid && p.batteryPercent == lastBatteryDrawn && p.loadWatts == lastLoadDrawn) {
        return;
    }
    lastPowerValid = p.valid;
    lastBatteryDrawn = p.batteryPercent;
    lastLoadDrawn = p.loadWatts;

    tft->fillRect(0, 0, 320, 26, TFT_BLACK);
    if (!p.valid) {
        return;
    }

    uint16_t battColor = batteryColor(*tft, p.batteryPercent);
    drawBatteryIcon(*tft, 10, 7, p.batteryPercent, battColor);

    char buf[16];
    tft->setTextDatum(ML_DATUM);
    tft->setTextColor(battColor, TFT_BLACK);
    snprintf(buf, sizeof(buf), "%d%%", p.batteryPercent);
    tft->drawString(buf, 10 + 22 + 8, 13, 4);

    snprintf(buf, sizeof(buf), "%dW", p.loadWatts);
    tft->setTextDatum(MR_DATUM);
    tft->setTextColor(TFT_ORANGE, TFT_BLACK);
    int rw = tft->drawString(buf, 305, 13, 4);
    drawBoltIcon(*tft, 305 - rw - 14, 13, TFT_ORANGE);

    tft->setTextDatum(TL_DATUM);
}

void ClockWeatherPage::drawTime(const struct tm &timeinfo) {
    if (timeinfo.tm_hour == lastDrawnHour && timeinfo.tm_min == lastDrawnMinute) {
        return;
    }
    lastDrawnHour = timeinfo.tm_hour;
    lastDrawnMinute = timeinfo.tm_min;

    tft->fillRect(0, 28, 320, 102, TFT_BLACK);

    int hour12 = timeinfo.tm_hour % 12;
    if (hour12 == 0) {
        hour12 = 12;
    }
    char buf[8];
    snprintf(buf, sizeof(buf), "%d:%02d", hour12, timeinfo.tm_min);
    const char *ampm = timeinfo.tm_hour < 12 ? "AM" : "PM";

    // Center the whole "time + AM/PM" group instead of anchoring it to a
    // fixed left edge -- a fixed anchor made the group visibly jump right
    // whenever the hour went from one digit to two (9:01 vs 10:00).
    tft->setTextSize(2);
    int timeW = tft->textWidth(buf, 7);
    tft->setTextSize(1);
    int ampmW = tft->textWidth(ampm, 4);
    constexpr int GAP = 8;
    constexpr int RIGHT_MARGIN = 6;
    // True bounding-box center reads as slightly left, since the eye
    // weighs the big digits more than the small AM/PM suffix -- nudged
    // right a bit to compensate, but clamped so a wide time (e.g. "12:59")
    // can't push the AM/PM suffix past the right edge of the panel.
    int startX = (320 - (timeW + GAP + ampmW)) / 2 + 10;
    int maxStartX = 320 - RIGHT_MARGIN - ampmW - GAP - timeW;
    startX = min(startX, maxStartX);

    tft->setTextColor(nightMode ? dimRed(*tft) : TFT_CYAN, TFT_BLACK);
    tft->setTextDatum(ML_DATUM);
    tft->setTextSize(2);
    tft->drawString(buf, startX, 85, 7);
    tft->setTextSize(1);

    tft->drawString(ampm, startX + timeW + GAP, 85, 4);
    tft->setTextDatum(TL_DATUM);
}

void ClockWeatherPage::drawDate(const struct tm &timeinfo) {
    if (timeinfo.tm_yday == lastDrawnYday) {
        return;
    }
    lastDrawnYday = timeinfo.tm_yday;

    tft->fillRect(0, 130, 320, 30, TFT_BLACK);
    char buf[40];
    strftime(buf, sizeof(buf), "%A, %B %d", &timeinfo);

    tft->setTextColor(nightMode ? dimRed(*tft) : TFT_GREEN, TFT_BLACK);
    tft->setTextDatum(MC_DATUM);
    tft->drawString(buf, 160, 145, 4);
    tft->setTextDatum(TL_DATUM);

    tft->drawFastHLine(90, 164, 140, TFT_DARKGREY);
}

void ClockWeatherPage::drawWeather() {
    const WeatherData &w = weather->data();
    int roundedTemp = static_cast<int>(lroundf(w.currentTempF));

    if (w.valid == lastWeatherValid && roundedTemp == lastTempDrawn && w.weatherCode == lastWeatherCodeDrawn) {
        return;
    }
    lastWeatherValid = w.valid;
    lastTempDrawn = roundedTemp;
    lastWeatherCodeDrawn = w.weatherCode;

    tft->fillRect(0, WEATHER_Y0, 320, WEATHER_H, TFT_BLACK);

    if (!w.valid) {
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->setTextDatum(MC_DATUM);
        tft->drawString("Weather unavailable", 160, WEATHER_Y0 + WEATHER_H / 2, 4);
        tft->setTextDatum(TL_DATUM);
        return;
    }

    int cy = WEATHER_Y0 + WEATHER_H / 2;
    // Nudged up a few px -- the icon's lowest strokes (cloud base / rain
    // drops) were sitting right at the bottom edge of the panel.
    int iconOverride = nightMode ? static_cast<int>(dimRed(*tft)) : -1;
    drawWeatherIcon(*tft, weatherCodeToIcon(w.weatherCode), 95, cy - 6, 2, iconOverride);

    char buf[16];
    snprintf(buf, sizeof(buf), "%dF", roundedTemp);
    tft->setTextColor(nightMode ? dimRed(*tft) : TFT_WHITE, TFT_BLACK);
    tft->setTextDatum(ML_DATUM);
    tft->setTextSize(2);
    tft->drawString(buf, 155, cy - 10, 4);
    tft->setTextSize(1);

    tft->setTextColor(nightMode ? dimRed(*tft) : TFT_SILVER, TFT_BLACK);
    tft->drawString(weatherCodeToText(w.weatherCode), 155, cy + 20, 4);
    tft->setTextDatum(TL_DATUM);
}

void ClockWeatherPage::setNightMode(bool night) {
    if (night == nightMode) {
        return;
    }
    nightMode = night;
    // Force the clock/date/weather text to redraw in the new color scheme
    // even though the underlying values haven't changed.
    lastDrawnHour = -1;
    lastDrawnYday = -1;
    lastTempDrawn = -9999;
    lastWeatherCodeDrawn = -1;
}

void ClockWeatherPage::loop() {
    drawPowerRow();

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 100)) {
        return;
    }

    drawTime(timeinfo);
    drawDate(timeinfo);
    drawWeather();
}
