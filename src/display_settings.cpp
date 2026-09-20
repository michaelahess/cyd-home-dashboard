#include "display_settings.h"

#include <Arduino.h>
#include <Preferences.h>

namespace DisplaySettings {
namespace {
bool loaded = false;
bool cached = false;
}  // namespace

bool flipped() {
    if (!loaded) {
        Preferences prefs;
        prefs.begin("cyd", true);
        cached = prefs.getBool("flipped", false);
        prefs.end();
        loaded = true;
    }
    return cached;
}

void toggleAndReboot() {
    bool next = !flipped();
    Preferences prefs;
    prefs.begin("cyd", false);
    prefs.putBool("flipped", next);
    prefs.end();
    ESP.restart();
}
}  // namespace DisplaySettings
