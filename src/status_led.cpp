#include "status_led.h"

#include <Arduino.h>

namespace {
// GPIO16/17 confirmed swapped from the originally assumed G/B assignment
// via a live one-GPIO-at-a-time test (16 -> blue, 17 -> green) -- that
// assumption was only ever sourced from web research for this board model,
// never independently verified until now. GPIO4 (red) initially looked
// wrong too (a blue/green blend instead of red in that same test), but an
// isolated blink test confirmed it: GPIO4 does drive a real red channel,
// it's just noticeably dimmer than blue/green at the same PWM duty --
// likely a real difference in the LED package's red die/current-limiting,
// not a wiring or software issue (full duty is already maximum current,
// so there's no further software headroom to brighten it).
constexpr int PIN_R = 4;
constexpr int PIN_G = 17;
constexpr int PIN_B = 16;
constexpr int PWM_FREQ_HZ = 5000;
constexpr int PWM_RES_BITS = 8;
constexpr int PWM_MAX = 255;
}  // namespace

void StatusLed::begin() {
    ledcAttach(PIN_R, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcAttach(PIN_G, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcAttach(PIN_B, PWM_FREQ_HZ, PWM_RES_BITS);
    apply();
}

void StatusLed::setColor(StatusColor c) {
    color = c;
    apply();
}

void StatusLed::setBrightness(float fraction) {
    brightness = constrain(fraction, 0.0f, 1.0f);
    apply();
}

void StatusLed::apply() {
    bool r = false, g = false, b = false;
    switch (color) {
        case StatusColor::RED:
            r = true;
            break;
        case StatusColor::GREEN:
            g = true;
            break;
        case StatusColor::BLUE:
            b = true;
            break;
        case StatusColor::PURPLE:
            r = true;
            b = true;
            break;
        case StatusColor::OFF:
        default:
            break;
    }
    // Active-low: a channel reads brightest when the pin spends all its
    // time LOW, so an "on" channel's duty (fraction of time HIGH) is
    // (1 - brightness); an "off" channel stays fully HIGH no matter what
    // brightness is set to.
    int onDuty = static_cast<int>((1.0f - brightness) * PWM_MAX);
    ledcWrite(PIN_R, r ? onDuty : PWM_MAX);
    ledcWrite(PIN_G, g ? onDuty : PWM_MAX);
    ledcWrite(PIN_B, b ? onDuty : PWM_MAX);
}
