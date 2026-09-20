#pragma once

// Persisted (NVS) percentage used to compute both the night backlight duty
// and the night status-LED brightness -- one user-facing "how dim at
// night" knob instead of two separate ones, since in practice they're
// always tuned together. Shared by main.cpp's loop() and the Settings page.
namespace NightSettings {
constexpr int MIN_PERCENT = 3;
constexpr int MAX_PERCENT = 40;
constexpr int STEP_PERCENT = 3;

int percent();            // loads from NVS on first call, cached after
void setPercent(int pct);  // clamped to [MIN_PERCENT, MAX_PERCENT], persists immediately
}  // namespace NightSettings
