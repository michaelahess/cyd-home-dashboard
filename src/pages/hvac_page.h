#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "beep.h"
#include "hvac_zones.h"
#include "page.h"
#include "weather.h"

// List of zones; tapping one opens a control screen for just that zone
// (mode buttons, setpoint +/-, fan-mode cycle), with a back button to
// return to the list.
class HvacPage : public Page {
public:
    void begin(TFT_eSPI &tftRef, HvacZonesManager &hvacRef, Beeper &beeperRef, const WeatherManager &weatherRef);

    void onShow() override;
    void loop() override;
    void onTap(int x, int y) override;

    // The per-zone control view packs in enough real buttons (back, 4 mode
    // buttons, setpoint +/-, fan cycle) that an accidental tap near the
    // top/bottom edge -- especially Back at the top and Fan at the bottom
    // -- used to silently change pages instead of hitting the intended
    // control. Disabling page-nav here makes Back the only way out.
    bool blocksPageNav() const override { return mode == Mode::DETAIL; }

private:
    enum class Mode { LIST, DETAIL };

    TFT_eSPI *tft = nullptr;
    HvacZonesManager *hvac = nullptr;
    Beeper *beeper = nullptr;
    const WeatherManager *weather = nullptr;
    uint32_t lastDrawMs = 0;
    Mode mode = Mode::LIST;
    int selectedZone = -1;

    bool hasLastListDrawn = false;
    HvacZoneData lastListDrawn[HvacZonesManager::NUM_ZONES];
    bool hasLastDetailDrawn = false;
    HvacZoneData lastDetailDrawn;

    void drawListRow(int y, const HvacZoneData &z);
    void drawList();
    void handleListTap(int x, int y);

    void drawDetail();
    void handleDetailTap(int x, int y);

    static bool sameValues(const HvacZoneData &a, const HvacZoneData &b);
};
