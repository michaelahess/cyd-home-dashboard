#pragma once

// Reads the CYD's onboard LDR (GPIO34). Safe to read at any time -- this is
// a passive analog input, unlike the backlight (GPIO21), which cannot be
// safely PWM-dimmed on this hardware (confirmed: any duty below 100%
// corrupts the display). This can only trigger *software* dimming (darker
// rendered colors), not actual backlight brightness control.
//
// NOT YET CALIBRATED: the LDR's baseline is contaminated by this board's
// own always-on backlight (can't be turned off first to get a clean
// reading, for the same PWM reason above), and it's unverified whether
// darkness reads as a higher or lower raw value on this wiring. loop()
// logs the raw/smoothed value every sample so the threshold and direction
// can be tuned from real day/night readings instead of guessed further.
class LightSensor {
public:
    void begin();
    void loop();

    bool isDark() const;
    int smoothedValue() const { return smoothed; }

private:
    int smoothed = -1;
    unsigned long lastSampleMs = 0;
};
