#include "settings_page.h"

#include "../display_settings.h"
#include "../night_settings.h"

namespace {
// Layout stays inside the 24..216 band main.cpp reserves globally for
// top/bottom page navigation before onTap() ever runs.
constexpr int BTN_X = 20, BTN_W = 280;
constexpr int CAL_BTN_Y = 32, CAL_BTN_H = 36;
constexpr int ROTATE_BTN_Y = CAL_BTN_Y + CAL_BTN_H + 8, ROTATE_BTN_H = 36;

constexpr int STEP_ROW_H = 40;
constexpr int NIGHT_ROW_Y = ROTATE_BTN_Y + ROTATE_BTN_H + 14;
constexpr int VOLUME_ROW_Y = NIGHT_ROW_Y + STEP_ROW_H + 10;
constexpr int MINUS_X = 20, STEP_BTN_W = 50;
constexpr int PLUS_X = 250;

bool hitTest(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

void drawButton(TFT_eSPI &tft, int y, int h, const char *label) {
    tft.drawRoundRect(BTN_X, y, BTN_W, h, 4, TFT_DARKGREY);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(label, 160, y + h / 2, 4);
}

// A labeled value with -/+ step buttons on either side, e.g. "Night
// Dimming: 12%".
void drawStepRow(TFT_eSPI &tft, int y, const String &label) {
    tft.drawRoundRect(MINUS_X, y, STEP_BTN_W, STEP_ROW_H, 4, TFT_DARKGREY);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("-", MINUS_X + STEP_BTN_W / 2, y + STEP_ROW_H / 2, 4);

    tft.drawRoundRect(PLUS_X, y, STEP_BTN_W, STEP_ROW_H, 4, TFT_DARKGREY);
    tft.drawString("+", PLUS_X + STEP_BTN_W / 2, y + STEP_ROW_H / 2, 4);

    tft.fillRect(MINUS_X + STEP_BTN_W + 4, y, PLUS_X - (MINUS_X + STEP_BTN_W + 4), STEP_ROW_H, TFT_BLACK);
    tft.drawString(label, 160, y + STEP_ROW_H / 2, 4);
}
}  // namespace

void SettingsPage::begin(TFT_eSPI &tftRef, TouchTap &touchRef, Beeper &beeperRef) {
    tft = &tftRef;
    touch = &touchRef;
    beeper = &beeperRef;
}

void SettingsPage::onShow() {
    draw();
}

void SettingsPage::draw() {
    tft->fillScreen(TFT_BLACK);
    tft->setTextDatum(TC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("Settings", 160, 4, 4);
    tft->setTextDatum(TL_DATUM);

    drawButton(*tft, CAL_BTN_Y, CAL_BTN_H, "Recalibrate Touch");
    drawButton(*tft, ROTATE_BTN_Y, ROTATE_BTN_H, "Rotate Display 180");

    char buf[24];
    snprintf(buf, sizeof(buf), "Night Dimming: %d%%", NightSettings::percent());
    drawStepRow(*tft, NIGHT_ROW_Y, buf);

    snprintf(buf, sizeof(buf), "Beep Volume: %d%%", beeper->volumePercent());
    drawStepRow(*tft, VOLUME_ROW_Y, buf);

    tft->setTextDatum(TL_DATUM);
}

void SettingsPage::onTap(int x, int y) {
    if (hitTest(x, y, BTN_X, CAL_BTN_Y, BTN_W, CAL_BTN_H)) {
        // Blocking, modal flow -- draws its own screen and returns once done.
        touch->calibrate(*tft);
        draw();
        return;
    }
    if (hitTest(x, y, BTN_X, ROTATE_BTN_Y, BTN_W, ROTATE_BTN_H)) {
        DisplaySettings::toggleAndReboot();  // never returns
        return;
    }
    if (hitTest(x, y, MINUS_X, NIGHT_ROW_Y, STEP_BTN_W, STEP_ROW_H)) {
        NightSettings::setPercent(NightSettings::percent() - NightSettings::STEP_PERCENT);
        beeper->beep();
        draw();
        return;
    }
    if (hitTest(x, y, PLUS_X, NIGHT_ROW_Y, STEP_BTN_W, STEP_ROW_H)) {
        NightSettings::setPercent(NightSettings::percent() + NightSettings::STEP_PERCENT);
        beeper->beep();
        draw();
        return;
    }
    if (hitTest(x, y, MINUS_X, VOLUME_ROW_Y, STEP_BTN_W, STEP_ROW_H)) {
        beeper->setVolumePercent(beeper->volumePercent() - 5);
        beeper->beep();
        draw();
        return;
    }
    if (hitTest(x, y, PLUS_X, VOLUME_ROW_Y, STEP_BTN_W, STEP_ROW_H)) {
        beeper->setVolumePercent(beeper->volumePercent() + 5);
        beeper->beep();
        draw();
        return;
    }
}

void SettingsPage::loop() {
    // Everything on this page only changes in response to a tap (handled
    // immediately in onTap), so there's nothing to poll here.
}
