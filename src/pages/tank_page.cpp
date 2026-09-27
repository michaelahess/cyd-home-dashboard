#include "tank_page.h"

namespace {
constexpr int ROW_Y0 = 28;
constexpr int ROWS_BOTTOM = 216;
constexpr uint32_t REDRAW_INTERVAL_MS = 1000;
}  // namespace

void TankPage::begin(TFT_eSPI &tftRef, const TankManager &tanksRef, int startIndexIn, int countIn) {
    tft = &tftRef;
    tanks = &tanksRef;
    startIndex = startIndexIn;
    count = countIn;
}

void TankPage::onShow() {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->setTextDatum(TC_DATUM);
    tft->drawString("Tank Temps", 160, 4, 4);
    tft->setTextDatum(TL_DATUM);
    lastDrawMs = 0;
    hasLastDrawn = false;
}

void TankPage::drawRow(int rowIndex, const TankData &t) {
    int rowH = (ROWS_BOTTOM - ROW_Y0) / count;
    int y = ROW_Y0 + rowIndex * rowH;

    tft->fillRect(0, y, 320, rowH, TFT_BLACK);
    tft->drawFastHLine(10, y + rowH - 1, 300, TFT_DARKGREY);

    int cy = y + rowH / 2;
    tft->setTextDatum(ML_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString(t.label, 14, cy, 4);

    tft->setTextDatum(MR_DATUM);
    if (t.valid) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1fF", t.tempF);
        tft->setTextColor(TFT_CYAN, TFT_BLACK);
        tft->drawString(buf, 306, cy, 4);
    } else {
        tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft->drawString("--", 306, cy, 4);
    }
    tft->setTextDatum(TL_DATUM);
}

void TankPage::drawAll() {
    for (int i = 0; i < count; i++) {
        drawRow(i, tanks->data(startIndex + i));
    }
}

void TankPage::loop() {
    uint32_t now = millis();
    if (hasLastDrawn && (now - lastDrawMs) < REDRAW_INTERVAL_MS) {
        return;
    }
    lastDrawMs = now;

    bool changed = !hasLastDrawn;
    TankData current[TankManager::NUM_TANKS];
    for (int i = 0; i < count; i++) {
        current[i] = tanks->data(startIndex + i);
        if (hasLastDrawn) {
            const TankData &prev = lastDrawn[i];
            if (prev.valid != current[i].valid || prev.tempF != current[i].tempF) {
                changed = true;
            }
        }
    }
    if (!changed) {
        return;
    }
    hasLastDrawn = true;
    for (int i = 0; i < count; i++) {
        lastDrawn[i] = current[i];
    }
    drawAll();
}
