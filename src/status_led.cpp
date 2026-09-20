#include "status_led.h"

#include <Arduino.h>

namespace {
constexpr int PIN_R = 4;
constexpr int PIN_G = 16;
constexpr int PIN_B = 17;
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
