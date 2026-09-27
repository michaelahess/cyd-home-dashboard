#include "pump_page.h"

#include <time.h>

namespace {
constexpr uint32_t REDRAW_INTERVAL_MS = 1000;
constexpr int ROW_Y0 = 28, ROW_H = 44;

void drawRow(TFT_eSPI &tft, int y, const char *label, const String &value, uint16_t color) {
    tft.fillRect(0, y, 320, ROW_H, TFT_BLACK);
    tft.drawFastHLine(10, y + ROW_H - 1, 300, TFT_DARKGREY);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(label, 14, y + 6, 2);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(value, 306, y + ROW_H / 2 + 8, 4);
    tft.setTextDatum(TL_DATUM);
}

// "9:05 AM" style, from 24h hour/minute already in local time.
String formatClock(int hour, int minute) {
    int hour12 = hour % 12;
    if (hour12 == 0) {
        hour12 = 12;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%d:%02d %s", hour12, minute, hour < 12 ? "AM" : "PM");
    return String(buf);
}
}  // namespace

void PumpPage::begin(TFT_eSPI &tftRef, const FishPumpManager &pumpRef) {
    tft = &tftRef;
    pump = &pumpRef;
}

void PumpPage::onShow() {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->setTextDatum(TC_DATUM);
    tft->drawString("Fish Pump", 160, 4, 4);
    tft->setTextDatum(TL_DATUM);
    lastDrawMs = 0;
    hasLastDrawn = false;
}

bool PumpPage::sameValues(const FishPumpData &a, const FishPumpData &b) {
    return a.valid == b.valid && a.runsToday == b.runsToday && a.lastRunHour == b.lastRunHour &&
           a.lastRunMinute == b.lastRunMinute && a.nextRunActive == b.nextRunActive &&
           a.nextRunEpoch == b.nextRunEpoch && a.wet == b.wet;
}

void PumpPage::drawAll() {
    const FishPumpData &p = pump->data();

    if (!p.valid) {
        tft->fillRect(0, ROW_Y0, 320, 4 * ROW_H, TFT_BLACK);
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->setTextDatum(MC_DATUM);
        tft->drawString("Pump data unavailable", 160, ROW_Y0 + 2 * ROW_H, 4);
        tft->setTextDatum(TL_DATUM);
        return;
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", p.runsToday);
    drawRow(*tft, ROW_Y0, "RUNS TODAY", buf, TFT_WHITE);

    drawRow(*tft, ROW_Y0 + ROW_H, "LAST RUN", formatClock(p.lastRunHour, p.lastRunMinute), TFT_WHITE);

    String nextRun = "--";
    if (p.nextRunActive && p.nextRunEpoch > 0) {
        time_t t = static_cast<time_t>(p.nextRunEpoch);
        struct tm tmNext;
        localtime_r(&t, &tmNext);
        nextRun = formatClock(tmNext.tm_hour, tmNext.tm_min);
    }
    drawRow(*tft, ROW_Y0 + 2 * ROW_H, "NEXT RUN", nextRun, TFT_WHITE);

    uint16_t wetColor = tft->color565(0x2a, 0x8f, 0xd8);          // blue -- has fluid
    uint16_t dryColor = tft->color565(0xd0, 0x3b, 0x3b);          // red -- reservoir empty
    drawRow(*tft, ROW_Y0 + 3 * ROW_H, "STATUS", p.wet ? "WET" : "DRY", p.wet ? wetColor : dryColor);
}

void PumpPage::loop() {
    uint32_t now = millis();
    if (hasLastDrawn && (now - lastDrawMs) < REDRAW_INTERVAL_MS) {
        return;
    }
    lastDrawMs = now;

    FishPumpData current = pump->data();
    if (hasLastDrawn && sameValues(current, lastDrawn)) {
        return;
    }
    hasLastDrawn = true;
    lastDrawn = current;
    drawAll();
}
