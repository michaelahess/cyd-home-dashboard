#include "beep.h"

#include <Arduino.h>
#include <Preferences.h>

namespace {
constexpr int BEEP_PIN = 26;
constexpr int BEEP_RES_BITS = 8;
// Duty is the fraction of each PWM cycle the pin is driven high, which
// (for this plain square-wave drive with no separate amplitude control) is
// what actually controls perceived loudness into the speaker pad. Capped
// well under 255 (100%) since 50% was reported "very loud" on the speaker
// this was tuned against -- a different speaker/amp on someone else's
// board is exactly why this is a runtime setting instead of a constant.
constexpr int MAX_DUTY = 60;
constexpr int DEFAULT_VOLUME_PERCENT = 8;  // -> duty ~5, the quiet default this was tuned to
}  // namespace

int Beeper::currentDuty() const {
    return (volumePct * MAX_DUTY) / 100;
}

void Beeper::begin() {
    ledcAttach(BEEP_PIN, 1800, BEEP_RES_BITS);
    Preferences prefs;
    prefs.begin("cyd", true);
    volumePct = prefs.getInt("beepvol", DEFAULT_VOLUME_PERCENT);
    prefs.end();
}

int Beeper::volumePercent() const {
    return volumePct;
}

void Beeper::setVolumePercent(int percent) {
    volumePct = constrain(percent, 0, 100);
    Preferences prefs;
    prefs.begin("cyd", false);
    prefs.putInt("beepvol", volumePct);
    prefs.end();
}

void Beeper::beep() {
    static const Note note = {1800, 35};
    startSequence(&note, 1);
}

void Beeper::playHappyTone() {
    static const Note notes[2] = {{1400, 55}, {2100, 85}};
    startSequence(notes, 2);
}

void Beeper::startSequence(const Note *notes, int count) {
    if (volumePct <= 0) {
        return;  // muted
    }
    count = min(count, MAX_NOTES);
    for (int i = 0; i < count; i++) {
        sequence[i] = notes[i];
    }
    sequenceLen = count;
    startNote(0);
}

void Beeper::startNote(int index) {
    noteIndex = index;
    noteStartedMs = millis();
    ledcChangeFrequency(BEEP_PIN, sequence[index].freqHz, BEEP_RES_BITS);
    ledcWrite(BEEP_PIN, currentDuty());
}

void Beeper::loop() {
    if (noteIndex < 0) {
        return;
    }
    if (millis() - noteStartedMs >= sequence[noteIndex].durationMs) {
        int next = noteIndex + 1;
        if (next < sequenceLen) {
            startNote(next);
        } else {
            ledcWrite(BEEP_PIN, 0);
            noteIndex = -1;
        }
    }
}
