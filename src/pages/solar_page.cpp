#include "solar_page.h"

namespace {
constexpr uint32_t REDRAW_INTERVAL_MS = 1000;
}  // namespace

void SolarPage::begin(TFT_eSPI &tftRef, const HomePowerManager &powerRef) {
    tft = &tftRef;
    power = &powerRef;
}

void SolarPage::onShow() {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->setTextDatum(TC_DATUM);
    tft->drawString("Solar", 160, 4, 4);
    tft->setTextDatum(TL_DATUM);
    lastDrawMs = 0;
    hasLastDrawn = false;
}

bool SolarPage::sameValues(const HomePowerData &a, const HomePowerData &b) {
    return a.valid == b.valid && a.batteryPercent == b.batteryPercent && a.loadWatts == b.loadWatts &&
           a.solarWatts == b.solarWatts && a.dailyYieldKwh == b.dailyYieldKwh && a.gridWatts == b.gridWatts &&
           a.batteryPowerWatts == b.batteryPowerWatts && a.batteryStatus == b.batteryStatus;
}

void SolarPage::drawTile(int x, int y, int w, int h, const char *label, const String &value, uint16_t color) {
    tft->fillRect(x, y, w, h, TFT_BLACK);
    tft->drawRect(x, y, w, h, TFT_DARKGREY);
    tft->setTextDatum(TC_DATUM);
    tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft->drawString(label, x + w / 2, y + 4, 2);
    tft->setTextColor(color, TFT_BLACK);
    tft->drawString(value, x + w / 2, y + h / 2, 4);
    tft->setTextDatum(TL_DATUM);
}

void SolarPage::drawAll() {
    const HomePowerData &p = power->data();

    if (!p.valid) {
        tft->fillRect(0, 30, 320, 180, TFT_BLACK);
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->setTextDatum(MC_DATUM);
        tft->drawString("Solar data unavailable", 160, 120, 4);
        tft->setTextDatum(TL_DATUM);
        return;
    }

    constexpr int COL1_X = 8, COL2_X = 164, TILE_W = 148, TILE_H = 62;
    constexpr int ROW1_Y = 30, ROW2_Y = 97, ROW3_Y = 164;

    char buf[24];
    uint16_t green = tft->color565(0x0c, 0xa3, 0x0c);

    snprintf(buf, sizeof(buf), "%dW", p.solarWatts);
    drawTile(COL1_X, ROW1_Y, TILE_W, TILE_H, "CURRENT PRODUCTION", buf, TFT_YELLOW);

    snprintf(buf, sizeof(buf), "%.1fkWh", p.dailyYieldKwh);
    drawTile(COL2_X, ROW1_Y, TILE_W, TILE_H, "TODAY'S PRODUCTION", buf, TFT_YELLOW);

    snprintf(buf, sizeof(buf), "%d%%", p.batteryPercent);
    drawTile(COL1_X, ROW2_Y, TILE_W, TILE_H, "BATTERY LEVEL", buf, green);

    snprintf(buf, sizeof(buf), "%dW", p.loadWatts);
    drawTile(COL2_X, ROW2_Y, TILE_W, TILE_H, "HOUSE LOAD", buf, TFT_ORANGE);

    const char *gridLabel = p.gridWatts >= 0 ? "GRID IMPORT" : "GRID EXPORT";
    snprintf(buf, sizeof(buf), "%dW", abs(p.gridWatts));
    drawTile(COL1_X, ROW3_Y, TILE_W, TILE_H, gridLabel, buf, TFT_CYAN);

    snprintf(buf, sizeof(buf), "%dW", abs(p.batteryPowerWatts));
    drawTile(COL2_X, ROW3_Y, TILE_W, TILE_H, p.batteryStatus.length() ? p.batteryStatus.c_str() : "BATTERY POWER", buf, green);
}

void SolarPage::loop() {
    uint32_t now = millis();
    if (hasLastDrawn && (now - lastDrawMs) < REDRAW_INTERVAL_MS) {
        return;
    }
    lastDrawMs = now;

    HomePowerData current = power->data();
    if (hasLastDrawn && sameValues(current, lastDrawn)) {
        return;
    }
    hasLastDrawn = true;
    lastDrawn = current;
    drawAll();
}
