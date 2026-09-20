#include "light_sensor.h"

#include <Arduino.h>

namespace {
constexpr int LDR_PIN = 34;
constexpr unsigned long SAMPLE_INTERVAL_MS = 2000;

// PLACEHOLDER -- not measured yet. Direction (whether darkness reads higher
// or lower) and magnitude both need tuning from the serial log
// ("Light sensor raw=... smoothed=..." printed every sample) against real
// day/night conditions on this specific board.
constexpr int DARK_THRESHOLD = 800;
constexpr bool DARK_IS_BELOW_THRESHOLD = true;  // flip if readings turn out inverted
}  // namespace

void LightSensor::begin() {
    pinMode(LDR_PIN, INPUT);
    smoothed = analogRead(LDR_PIN);
}

void LightSensor::loop() {
    unsigned long now = millis();
    if (now - lastSampleMs < SAMPLE_INTERVAL_MS) {
        return;
    }
    lastSampleMs = now;

    int raw = analogRead(LDR_PIN);
    smoothed = (smoothed < 0) ? raw : (smoothed * 3 + raw) / 4;
    Serial.printf("Light sensor raw=%d smoothed=%d dark=%d\n", raw, smoothed, isDark());
}

bool LightSensor::isDark() const {
    // A flat 0 (seen consistently on the currently-tested board, regardless
    // of actual room lighting) reads as an invalid/disconnected sensor, not
    // genuine darkness -- a real reading, even in a dark room, would show
    // some noise rather than an exact, unwavering 0. Trusting a stuck 0
    // here would keep the display dimmed/night-colored all day, every day.
    if (smoothed <= 0) {
        return false;
    }
    return DARK_IS_BELOW_THRESHOLD ? (smoothed < DARK_THRESHOLD) : (smoothed > DARK_THRESHOLD);
}
