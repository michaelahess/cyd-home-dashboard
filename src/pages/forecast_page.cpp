#include "forecast_page.h"

#include "weather_icon.h"

namespace {
constexpr int ROW_Y0 = 34;
constexpr int ROW_H = 41;
constexpr uint16_t STRIPE_COLOR = 0x18E3;  // faint dark blue-grey, barely-there row banding
}  // namespace

void ForecastPage::begin(TFT_eSPI &tftRef, const WeatherManager &weatherRef) {
    tft = &tftRef;
    weather = &weatherRef;
}

void ForecastPage::onShow() {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->setTextDatum(TC_DATUM);
    tft->drawString("5-Day Forecast", 160, 6, 4);
    tft->drawFastHLine(60, 30, 200, TFT_DARKGREY);
    tft->setTextDatum(TL_DATUM);
    drawnOnce = false;
    lastValid = false;
}

void ForecastPage::drawRow(int rowIndex, const DailyForecast &day) {
    int y = ROW_Y0 + rowIndex * ROW_H;
    uint16_t bg = (rowIndex % 2 == 1) ? STRIPE_COLOR : TFT_BLACK;
    tft->fillRect(0, y, 320, ROW_H, bg);

    int cy = y + ROW_H / 2;

    tft->setTextColor(TFT_WHITE, bg);
    tft->setTextDatum(ML_DATUM);
    tft->drawString(day.dayLabel, 14, cy, 4);

    drawWeatherIcon(*tft, weatherCodeToIcon(day.weatherCode), 100, cy, 1);

    char buf[24];
    snprintf(buf, sizeof(buf), "%d / %d F", static_cast<int>(lroundf(day.tempMaxF)), static_cast<int>(lroundf(day.tempMinF)));
    tft->setTextDatum(MC_DATUM);
    tft->drawString(buf, 195, cy, 4);

    tft->setTextColor(TFT_SKYBLUE, bg);
    snprintf(buf, sizeof(buf), "%d%%", day.precipProbability);
    tft->setTextDatum(MR_DATUM);
    tft->drawString(buf, 305, cy, 4);

    tft->setTextDatum(TL_DATUM);
}

void ForecastPage::drawAll() {
    const WeatherData &w = weather->data();
    drawnOnce = true;
    lastValid = w.valid;

    if (!w.valid) {
        tft->fillRect(0, ROW_Y0, 320, WeatherData::NUM_DAYS * ROW_H, TFT_BLACK);
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->setTextDatum(MC_DATUM);
        tft->drawString("Weather unavailable", 160, ROW_Y0 + 80, 4);
        tft->setTextDatum(TL_DATUM);
        return;
    }

    for (int i = 0; i < WeatherData::NUM_DAYS; i++) {
        drawRow(i, w.daily[i]);
    }
}

void ForecastPage::loop() {
    if (!drawnOnce || weather->data().valid != lastValid) {
        drawAll();
    }
}
