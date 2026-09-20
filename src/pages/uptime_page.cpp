#include "uptime_page.h"

namespace {
constexpr uint32_t REDRAW_INTERVAL_MS = 1000;
}  // namespace

void UptimePage::begin(TFT_eSPI &tftRef, const UptimeKumaManager &kumaRef) {
    tft = &tftRef;
    kuma = &kumaRef;
}

void UptimePage::onShow() {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->setTextDatum(TC_DATUM);
    tft->drawString("System Status", 160, 4, 4);
    tft->setTextDatum(TL_DATUM);
    lastDrawMs = 0;
    hasLastDrawn = false;
}

void UptimePage::drawAll() {
    tft->fillRect(0, 30, 320, 210, TFT_BLACK);

    if (!kuma->configLoaded()) {
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->setTextDatum(MC_DATUM);
        tft->drawString("Loading...", 160, 120, 4);
        tft->setTextDatum(TL_DATUM);
        return;
    }

    int total = kuma->monitorCount();
    int down = kuma->downCount();
    int up = total - down;
    uint16_t green = tft->color565(0x0c, 0xa3, 0x0c);
    uint16_t red = tft->color565(0xd0, 0x3b, 0x3b);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d / %d Up", up, total);
    tft->setTextDatum(TC_DATUM);
    tft->setTextColor(down == 0 ? green : TFT_WHITE, TFT_BLACK);
    tft->drawString(buf, 160, 40, 4);

    if (down == 0) {
        // Happy checkmark: green circle with a white check inside.
        constexpr int CX = 160, CY = 130, R = 45;
        tft->fillCircle(CX, CY, R, green);
        tft->drawLine(CX - 20, CY, CX - 5, CY + 17, TFT_WHITE);
        tft->drawLine(CX - 19, CY + 1, CX - 4, CY + 18, TFT_WHITE);
        tft->drawLine(CX - 5, CY + 17, CX + 24, CY - 17, TFT_WHITE);
        tft->drawLine(CX - 4, CY + 18, CX + 25, CY - 16, TFT_WHITE);
        tft->setTextDatum(TC_DATUM);
        tft->setTextColor(green, TFT_BLACK);
        tft->drawString("All Systems Operational", 160, 190, 4);
    } else {
        tft->setTextDatum(TL_DATUM);
        tft->setTextColor(red, TFT_BLACK);
        int y = 70;
        for (int i = 0; i < down && y < 230; i++) {
            tft->drawString(String("- ") + kuma->downName(i), 20, y, 4);
            y += 26;
        }
    }

    tft->setTextDatum(TL_DATUM);
}

void UptimePage::loop() {
    uint32_t now = millis();
    if (hasLastDrawn && (now - lastDrawMs) < REDRAW_INTERVAL_MS) {
        return;
    }
    lastDrawMs = now;

    bool configLoaded = kuma->configLoaded();
    int up = 0, down = 0;
    String downList;
    if (configLoaded) {
        int total = kuma->monitorCount();
        down = kuma->downCount();
        up = total - down;
        for (int i = 0; i < down; i++) {
            downList += kuma->downName(i) + "|";
        }
    }

    bool changed = !hasLastDrawn || configLoaded != lastConfigLoaded || up != lastUp || down != lastDown ||
                   downList != lastDownList;
    if (!changed) {
        return;
    }
    hasLastDrawn = true;
    lastConfigLoaded = configLoaded;
    lastUp = up;
    lastDown = down;
    lastDownList = downList;
    drawAll();
}
