#pragma once

// Short, non-blocking confirmation tones on the onboard speaker pad
// (GPIO26). Uses a plain LEDC square wave rather than the DAC -- adequate
// for button-feedback clicks/chimes, not meant for audio quality.
class Beeper {
public:
    void begin();

    // Short, quiet click -- used for most taps (list row select, back,
    // setpoint +/-, fan-mode cycle).
    void beep();

    // Two-note ascending chime -- used specifically for HVAC mode changes
    // (off/heat/cool/auto), since those actuate a real unit and deserve a
    // more distinct, deliberate sound than a plain tap click.
    void playHappyTone();

    // Call every main-loop iteration; advances/stops the current tone
    // sequence. Non-blocking (no delay()).
    void loop();

    // 0-100. Persisted to NVS immediately and applied to the next tone;
    // exposed so a Settings page can offer +/- volume control without
    // needing a reflash for every speaker/amp this ends up driving.
    int volumePercent() const;
    void setVolumePercent(int percent);

private:
    struct Note {
        int freqHz;
        unsigned long durationMs;
    };

    static constexpr int MAX_NOTES = 2;

    void startSequence(const Note *notes, int count);
    void startNote(int index);
    int currentDuty() const;

    int volumePct = 8;  // loaded from NVS in begin(); see DEFAULT_VOLUME_PERCENT
    Note sequence[MAX_NOTES] = {};
    int sequenceLen = 0;
    int noteIndex = -1;
    unsigned long noteStartedMs = 0;
};
