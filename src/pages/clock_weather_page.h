#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>

#include "page.h"
#include "weather.h"

// Large, minimal clock + date + current-conditions page. Deliberately
// sparse: no gridlines, labels, or touch hints -- just the numbers and one
// icon.
class ClockWeatherPage : public Page {
public:
    void begin(TFT_eSPI &tftRef, const WeatherManager &weatherRef);

    void onShow() override;
    void loop() override;

    // Called every main-loop iteration by main.cpp (which owns the combined
    // clock-hour + light-sensor decision, since that same decision also
    // drives backlight dimming -- not just this page's colors, and needs
    // to keep running even while a different page is visible).
    void setNightMode(bool night);

private:
    TFT_eSPI *tft = nullptr;
    const WeatherManager *weather = nullptr;

    int lastDrawnHour = -1;
    int lastDrawnMinute = -1;
    int lastDrawnYday = -1;
    bool lastWeatherValid = false;
    int lastTempDrawn = -9999;
    int lastWeatherCodeDrawn = -1;
    bool nightMode = false;

    void drawTime(const struct tm &timeinfo);
    void drawDate(const struct tm &timeinfo);
    void drawWeather();
};
