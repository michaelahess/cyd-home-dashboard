#pragma once

// Hubitat contact/window sensors. NOT YET ACTIVE: no window devices are
// exposed to the Maker API app yet (confirmed via /devices -- only the 5
// HVAC + 2 Tesla devices are there). Once you expose them in Hubitat and
// give me their device IDs, fill in SENSOR_IDS in windows_status.cpp (and
// bump NUM_SENSORS) -- everything else already works off hasSensors().
class WindowsStatusManager {
public:
    void loop();

    bool hasSensors() const;
    bool anyOpen() const;
    bool allClosed() const;  // only meaningful if hasSensors()

private:
    bool fetchNow();
};
