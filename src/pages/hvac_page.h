#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "beep.h"
#include "hvac_zones.h"
#include "page.h"

// List of 5 zones; tapping one opens a control screen for just that zone
// (mode buttons, setpoint +/-, fan-mode cycle), with a back button to
// return to the list.
class HvacPage : public Page {
public:
    void begin(TFT_eSPI &tftRef, HvacZonesManager &hvacRef, Beeper &beeperRef);

    void onShow() override;
    void loop() override;
    void onTap(int x, int y) override;

private:
    enum class Mode { LIST, DETAIL };

    TFT_eSPI *tft = nullptr;
    HvacZonesManager *hvac = nullptr;
    Beeper *beeper = nullptr;
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
