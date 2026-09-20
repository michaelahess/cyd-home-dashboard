#include "tesla_page.h"

namespace {
constexpr uint32_t REDRAW_INTERVAL_MS = 1000;
constexpr int BLOCK_H = 102;
}  // namespace

void TeslaPage::begin(TFT_eSPI &tftRef, const TeslaManager &teslaRef) {
    tft = &tftRef;
    tesla = &teslaRef;
}

void TeslaPage::onShow() {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->setTextDatum(TC_DATUM);
    tft->drawString("Tesla", 160, 4, 4);
    tft->setTextDatum(TL_DATUM);
    lastDrawMs = 0;
    hasLastDrawn = false;
}

bool TeslaPage::sameValues(const TeslaVehicleData &a, const TeslaVehicleData &b) {
    return a.valid == b.valid && a.batteryPercent == b.batteryPercent && a.insideTempF == b.insideTempF &&
           a.outsideTempF == b.outsideTempF && a.lockState == b.lockState && a.state == b.state &&
           a.present == b.present;
}

void TeslaPage::drawVehicle(int y, const TeslaVehicleData &v) {
    tft->fillRect(0, y, 320, BLOCK_H, TFT_BLACK);
    tft->drawFastHLine(10, y, 300, TFT_DARKGREY);

    if (!v.valid) {
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->setTextDatum(MC_DATUM);
        tft->drawString("Unavailable", 160, y + BLOCK_H / 2, 4);
        tft->setTextDatum(TL_DATUM);
        return;
    }

    uint16_t green = tft->color565(0x0c, 0xa3, 0x0c);

    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString(v.name, 14, y + 6, 4);

    tft->setTextDatum(TR_DATUM);
    tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft->drawString(v.state.length() ? v.state : "-", 306, y + 12, 2);

    char buf[24];
    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(green, TFT_BLACK);
    snprintf(buf, sizeof(buf), "%d%%", v.batteryPercent);
    tft->drawString(buf, 14, y + 34, 4);

    tft->setTextDatum(TR_DATUM);
    bool locked = v.lockState == "locked";
    tft->setTextColor(locked ? green : TFT_ORANGE, TFT_BLACK);
    tft->drawString(v.lockState.length() ? v.lockState : "-", 306, y + 40, 2);

    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(TFT_CYAN, TFT_BLACK);
    snprintf(buf, sizeof(buf), "In: %.0fF", v.insideTempF);
    tft->drawString(buf, 14, y + 66, 4);
    snprintf(buf, sizeof(buf), "Out: %.0fF", v.outsideTempF);
    tft->drawString(buf, 165, y + 66, 4);
}

void TeslaPage::drawAll() {
    drawVehicle(28, tesla->data(0));
    drawVehicle(28 + BLOCK_H + 6, tesla->data(1));
}

void TeslaPage::loop() {
    uint32_t now = millis();
    if (hasLastDrawn && (now - lastDrawMs) < REDRAW_INTERVAL_MS) {
        return;
    }
    lastDrawMs = now;

    TeslaVehicleData current[TeslaManager::NUM_VEHICLES];
    bool changed = !hasLastDrawn;
    for (int i = 0; i < TeslaManager::NUM_VEHICLES; i++) {
        current[i] = tesla->data(i);
        if (hasLastDrawn && !sameValues(current[i], lastDrawn[i])) {
            changed = true;
        }
    }
    if (!changed) {
        return;
    }
    hasLastDrawn = true;
    for (int i = 0; i < TeslaManager::NUM_VEHICLES; i++) {
        lastDrawn[i] = current[i];
    }
    drawAll();
}
