#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "page.h"
#include "weather.h"

// 5-day forecast: one row per day, weekday + simple hand-drawn condition
// icon + hi/lo + precipitation chance. Shares the same cached WeatherData
// the clock/weather page reads -- one data source, not two.
class ForecastPage : public Page {
public:
    void begin(TFT_eSPI &tftRef, const WeatherManager &weatherRef);

    void onShow() override;
    void loop() override;

private:
    TFT_eSPI *tft = nullptr;
    const WeatherManager *weather = nullptr;
    bool drawnOnce = false;
    bool lastValid = false;

    void drawAll();
    void drawRow(int rowIndex, const DailyForecast &day);
};
