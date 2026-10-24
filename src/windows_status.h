#pragma once

// Home Assistant contact/window sensors -- drives the status LED's blue
// (open) / green (all closed) state. Entity IDs are WINDOW_SENSOR_ENTITIES
// (and NUM_WINDOW_SENSORS) in secrets.h. Everything else
// here works off hasSensors(), so leaving NUM_SENSORS at 0 disables this
// cleanly.
class WindowsStatusManager {
public:
    void loop();

    bool hasSensors() const;
    bool anyOpen() const;
    bool allClosed() const;  // only meaningful if hasSensors()

private:
    bool fetchNow();
};
