#include "night_settings.h"

#include <Arduino.h>
#include <Preferences.h>

namespace NightSettings {
namespace {
constexpr int DEFAULT_PERCENT = 12;  // confirmed comfortable via on-device testing
bool loaded = false;
int cached = DEFAULT_PERCENT;
}  // namespace

int percent() {
    if (!loaded) {
        Preferences prefs;
        prefs.begin("cyd", true);
        cached = prefs.getInt("nightpct", DEFAULT_PERCENT);
        prefs.end();
        loaded = true;
    }
    return cached;
}

void setPercent(int pct) {
    cached = constrain(pct, MIN_PERCENT, MAX_PERCENT);
    loaded = true;
    Preferences prefs;
    prefs.begin("cyd", false);
    prefs.putInt("nightpct", cached);
    prefs.end();
}
}  // namespace NightSettings
