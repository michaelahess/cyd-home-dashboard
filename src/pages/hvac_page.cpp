#include "hvac_page.h"

namespace {
constexpr uint32_t REDRAW_INTERVAL_MS = 1000;
constexpr int ROW_Y0 = 32;
// 36, not 40 -- at 40, the last row (5 zones * 40 + 32 = 232) extended past
// y=216, main.cpp's bottom page-nav boundary, so a tap on the lower part of
// the last row silently changed pages instead of opening that zone. 36
// keeps all rows (32..212) clear of that boundary.
constexpr int ROW_H = 36;

// Detail-view layout. This view disables main.cpp's top/bottom
// page-navigation edge strips entirely (see HvacPage::blocksPageNav()) --
// Back is the only way out -- so it's free to use the full 0..240 height,
// including what would otherwise be dead/nav-only space at the very
// bottom. That reclaimed space is a footer showing outside temperature,
// which also puts a real target where an accidental low tap used to
// silently flip to the next page instead of hitting Fan.
constexpr int BACK_X = 0, BACK_Y = 24, BACK_W = 90, BACK_H = 30;
// Back's actual tap target is bigger than its drawn button and reaches all
// the way to the top edge -- that corner is otherwise empty (the zone-name
// title is centered) and page-nav is fully disabled on this screen anyway
// (see blocksPageNav()), so there's no downside to being generous here. A
// resistive panel's calibration is typically least accurate right at the
// extreme corners, and this is exactly that corner.
constexpr int BACK_HIT_X = 0, BACK_HIT_Y = 0, BACK_HIT_W = 110, BACK_HIT_H = 58;
constexpr int MODE_BTN_Y = 60, MODE_BTN_H = 40, MODE_BTN_W = 76, MODE_BTN_GAP = 4;
constexpr int SETPOINT_Y = 106, SETPOINT_H = 63;
constexpr int MINUS_X = 15, MINUS_W = 60;
constexpr int PLUS_X = 245, PLUS_W = 60;
constexpr int FAN_X = 60, FAN_Y = 175, FAN_W = 200, FAN_H = 30;
constexpr int FOOTER_Y = 210, FOOTER_H = 30;

const char *MODE_COMMANDS[4] = {"off", "heat", "cool", "auto"};
const char *MODE_LABELS[4] = {"OFF", "HEAT", "COOL", "AUTO"};

bool hitTest(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

uint16_t modeColorFor(TFT_eSPI &tft, const String &mode) {
    if (mode == "cool") {
        return TFT_CYAN;
    }
    if (mode == "heat" || mode == "emergency heat") {
        return TFT_ORANGE;
    }
    if (mode == "auto") {
        return tft.color565(0x0c, 0xa3, 0x0c);
    }
    return TFT_DARKGREY;
}
}  // namespace

void HvacPage::begin(TFT_eSPI &tftRef, HvacZonesManager &hvacRef, Beeper &beeperRef, const WeatherManager &weatherRef) {
    tft = &tftRef;
    hvac = &hvacRef;
    beeper = &beeperRef;
    weather = &weatherRef;
}

void HvacPage::onShow() {
    mode = Mode::LIST;
    selectedZone = -1;
    lastDrawMs = 0;
    hasLastListDrawn = false;
    hasLastDetailDrawn = false;
    drawList();
}

bool HvacPage::sameValues(const HvacZoneData &a, const HvacZoneData &b) {
    return a.valid == b.valid && a.label == b.label && a.mode == b.mode && a.currentTempF == b.currentTempF &&
           a.coolingSetpointF == b.coolingSetpointF && a.heatingSetpointF == b.heatingSetpointF &&
           a.fanMode == b.fanMode;
}

void HvacPage::drawListRow(int y, const HvacZoneData &z) {
    tft->fillRect(0, y, 320, ROW_H, TFT_BLACK);
    tft->drawFastHLine(10, y + ROW_H - 1, 300, TFT_DARKGREY);

    int cy = y + ROW_H / 2;
    tft->setTextDatum(ML_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString(z.label, 12, cy, 4);

    if (!z.valid) {
        tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft->setTextDatum(MR_DATUM);
        tft->drawString("-", 308, cy, 4);
        tft->setTextDatum(TL_DATUM);
        return;
    }

    String modeText = z.mode;
    modeText.toUpperCase();
    tft->setTextColor(modeColorFor(*tft, z.mode), TFT_BLACK);
    tft->drawString(modeText.length() ? modeText : "-", 110, cy, 4);

    char buf[24];
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->setTextDatum(MR_DATUM);
    if (z.mode == "cool") {
        snprintf(buf, sizeof(buf), "%.0f->%.0fF", z.currentTempF, z.coolingSetpointF);
    } else if (z.mode == "heat" || z.mode == "emergency heat") {
        snprintf(buf, sizeof(buf), "%.0f->%.0fF", z.currentTempF, z.heatingSetpointF);
    } else {
        snprintf(buf, sizeof(buf), "%.0fF", z.currentTempF);
    }
    tft->drawString(buf, 308, cy, 4);
    tft->setTextDatum(TL_DATUM);
}

void HvacPage::drawList() {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->setTextDatum(TC_DATUM);
    tft->drawString("HVAC", 160, 4, 4);
    tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft->drawString("tap a zone to control it", 160, 26, 2);
    tft->setTextDatum(TL_DATUM);

    for (int i = 0; i < HvacZonesManager::NUM_ZONES; i++) {
        drawListRow(ROW_Y0 + i * ROW_H, hvac->data(i));
    }
}

void HvacPage::handleListTap(int x, int y) {
    if (y < ROW_Y0) {
        return;
    }
    int index = (y - ROW_Y0) / ROW_H;
    if (index < 0 || index >= HvacZonesManager::NUM_ZONES) {
        return;
    }
    selectedZone = index;
    mode = Mode::DETAIL;
    hasLastDetailDrawn = false;
    drawDetail();
}

void HvacPage::drawDetail() {
    if (selectedZone < 0) {
        return;
    }
    HvacZoneData z = hvac->data(selectedZone);
    tft->fillScreen(TFT_BLACK);

    tft->setTextDatum(TC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString(z.label, 160, 4, 4);

    tft->setTextDatum(ML_DATUM);
    tft->setTextColor(TFT_CYAN, TFT_BLACK);
    tft->drawString("< Back", BACK_X + 8, BACK_Y + BACK_H / 2, 4);

    char buf[16];
    tft->setTextDatum(MR_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    snprintf(buf, sizeof(buf), "%.0fF now", z.currentTempF);
    tft->drawString(buf, 312, BACK_Y + BACK_H / 2, 2);

    for (int i = 0; i < 4; i++) {
        int bx = 4 + i * (MODE_BTN_W + MODE_BTN_GAP);
        bool active = (z.mode == MODE_COMMANDS[i]) || (i == 1 && z.mode == "emergency heat");
        uint16_t bg = active ? modeColorFor(*tft, z.mode) : TFT_BLACK;
        uint16_t fg = active ? TFT_BLACK : TFT_WHITE;
        tft->drawRect(bx, MODE_BTN_Y, MODE_BTN_W, MODE_BTN_H, TFT_DARKGREY);
        tft->fillRect(bx + 1, MODE_BTN_Y + 1, MODE_BTN_W - 2, MODE_BTN_H - 2, bg);
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(fg, bg);
        tft->drawString(MODE_LABELS[i], bx + MODE_BTN_W / 2, MODE_BTN_Y + MODE_BTN_H / 2, 4);
    }

    bool isCooling = (z.mode == "cool");
    bool isHeating = (z.mode == "heat" || z.mode == "emergency heat");
    bool adjustable = isCooling || isHeating;
    float setpoint = isCooling ? z.coolingSetpointF : z.heatingSetpointF;

    uint16_t setpointColor = adjustable ? TFT_WHITE : TFT_DARKGREY;
    tft->drawRect(MINUS_X, SETPOINT_Y, MINUS_W, SETPOINT_H, TFT_DARKGREY);
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(setpointColor, TFT_BLACK);
    tft->drawString("-", MINUS_X + MINUS_W / 2, SETPOINT_Y + SETPOINT_H / 2, 4);

    tft->drawRect(PLUS_X, SETPOINT_Y, PLUS_W, SETPOINT_H, TFT_DARKGREY);
    tft->drawString("+", PLUS_X + PLUS_W / 2, SETPOINT_Y + SETPOINT_H / 2, 4);

    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->setTextSize(2);
    if (adjustable) {
        snprintf(buf, sizeof(buf), "%.0fF", setpoint);
    } else {
        snprintf(buf, sizeof(buf), "--");
    }
    tft->drawString(buf, 160, SETPOINT_Y + SETPOINT_H / 2, 4);
    tft->setTextSize(1);

    tft->drawRect(FAN_X, FAN_Y, FAN_W, FAN_H, TFT_DARKGREY);
    String fanLabel = "FAN: " + z.fanMode;
    fanLabel.toUpperCase();
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString(fanLabel, 160, FAN_Y + FAN_H / 2, 4);

    tft->drawFastHLine(10, FOOTER_Y, 300, TFT_DARKGREY);
    tft->setTextColor(TFT_SILVER, TFT_BLACK);
    WeatherData w = weather->data();
    if (w.valid) {
        snprintf(buf, sizeof(buf), "Outside: %.0fF", w.currentTempF);
    } else {
        snprintf(buf, sizeof(buf), "Outside: --");
    }
    tft->drawString(buf, 160, FOOTER_Y + FOOTER_H / 2, 4);

    tft->setTextDatum(TL_DATUM);
}

void HvacPage::handleDetailTap(int x, int y) {
    if (selectedZone < 0) {
        return;
    }

    if (hitTest(x, y, BACK_HIT_X, BACK_HIT_Y, BACK_HIT_W, BACK_HIT_H)) {
        beeper->beep();
        mode = Mode::LIST;
        selectedZone = -1;
        hasLastListDrawn = false;
        drawList();
        return;
    }

    for (int i = 0; i < 4; i++) {
        int bx = 4 + i * (MODE_BTN_W + MODE_BTN_GAP);
        if (hitTest(x, y, bx, MODE_BTN_Y, MODE_BTN_W, MODE_BTN_H)) {
            // Distinct chime here (not the plain click) -- this is the
            // button that actually turns a unit on/off/heat/cool, more
            // consequential than the other controls on this screen.
            beeper->playHappyTone();
            hvac->sendMode(selectedZone, MODE_COMMANDS[i]);
            drawDetail();
            return;
        }
    }

    if (hitTest(x, y, MINUS_X, SETPOINT_Y, MINUS_W, SETPOINT_H)) {
        beeper->beep();
        hvac->adjustSetpoint(selectedZone, -1.0f);
        drawDetail();
        return;
    }
    if (hitTest(x, y, PLUS_X, SETPOINT_Y, PLUS_W, SETPOINT_H)) {
        beeper->beep();
        hvac->adjustSetpoint(selectedZone, 1.0f);
        drawDetail();
        return;
    }

    if (hitTest(x, y, FAN_X, FAN_Y, FAN_W, FAN_H)) {
        beeper->beep();
        hvac->cycleFanMode(selectedZone);
        drawDetail();
        return;
    }
}

void HvacPage::onTap(int x, int y) {
    if (mode == Mode::LIST) {
        beeper->beep();
        handleListTap(x, y);
    } else {
        handleDetailTap(x, y);
    }
}

void HvacPage::loop() {
    uint32_t now = millis();
    if (lastDrawMs != 0 && (now - lastDrawMs) < REDRAW_INTERVAL_MS) {
        return;
    }
    lastDrawMs = now;

    if (mode == Mode::LIST) {
        HvacZoneData current[HvacZonesManager::NUM_ZONES];
        bool changed = !hasLastListDrawn;
        for (int i = 0; i < HvacZonesManager::NUM_ZONES; i++) {
            current[i] = hvac->data(i);
            if (hasLastListDrawn && !sameValues(current[i], lastListDrawn[i])) {
                changed = true;
            }
        }
        if (!changed) {
            return;
        }
        hasLastListDrawn = true;
        for (int i = 0; i < HvacZonesManager::NUM_ZONES; i++) {
            lastListDrawn[i] = current[i];
        }
        drawList();
    } else {
        HvacZoneData current = hvac->data(selectedZone);
        if (hasLastDetailDrawn && sameValues(current, lastDetailDrawn)) {
            return;
        }
        hasLastDetailDrawn = true;
        lastDetailDrawn = current;
        drawDetail();
    }
}
