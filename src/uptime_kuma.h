#pragma once

#include <Arduino.h>

// Reads a public Uptime Kuma status page (base URL + slug from secrets.h)
// -- no auth needed, it's a published public status page. Two endpoints:
// the monitor list (id/name) barely ever changes so it's fetched once in
// begin(); current up/down per monitor is polled periodically. Optional --
// leaving UPTIME_KUMA_BASE_URL empty in secrets.h disables this entirely.
class UptimeKumaManager {
public:
    static constexpr int MAX_MONITORS = 24;

    void begin();
    void loop();

    // True if UPTIME_KUMA_BASE_URL (secrets.h) is non-empty.
    static bool configured();

    bool configLoaded() const;
    int monitorCount() const;
    int upCount() const;
    int downCount() const;
    // name of the i-th monitor that is currently down (0 <= i < downCount())
    String downName(int index) const;

private:
    struct Monitor {
        int id = 0;
        String name;
        bool up = true;
    };

    Monitor monitors[MAX_MONITORS];
    int numMonitors = 0;
    bool haveConfig = false;
    unsigned long lastStatusFetchMs = 0;

    bool fetchConfig();
    bool fetchStatuses();
};
