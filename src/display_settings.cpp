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

    // Touch calibration is captured live under whichever rotation is
    // active at the time (see TouchTap::calibrate()), so it's tied to that
    // orientation -- flipping the display invalidates it. Clearing it here
    // makes the post-reboot first-boot calibration flow (main.cpp) run
    // again automatically instead of leaving stale, now-wrong bounds in
    // place (which otherwise reads as touch being mirrored/inverted).
    Preferences touchPrefs;
    touchPrefs.begin("touch", false);
    touchPrefs.clear();
    touchPrefs.end();

    ESP.restart();
}
}  // namespace DisplaySettings
