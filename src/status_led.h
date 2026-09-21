#pragma once

enum class StatusColor {
    OFF,
    RED,
    GREEN,
    BLUE,
    PURPLE,
};

// Onboard RGB LED (LED1), simple 3-channel common type (not addressable),
// active-low on GPIO4 (red, unconfirmed -- see status_led.cpp) / GPIO17
// (green) / GPIO16 (blue). Driven via LEDC PWM (not plain digitalWrite) so
// brightness can be pulled down at night alongside the screen dimming,
// using the same night-mode signal main.cpp already computes for the
// backlight.
class StatusLed {
public:
    void begin();
    void setColor(StatusColor color);

    // 0.0 = fully off regardless of color, 1.0 = full brightness.
    void setBrightness(float fraction);

private:
    void apply();

    StatusColor color = StatusColor::OFF;
    float brightness = 1.0f;
};
