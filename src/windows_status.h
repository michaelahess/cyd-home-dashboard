#pragma once

// Hubitat contact/window sensors -- drives the status LED's blue (open) /
// green (all closed) state. Device IDs are in windows_status.cpp; edit
// SENSOR_IDS (and NUM_SENSORS) there for your own sensors. Everything else
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
