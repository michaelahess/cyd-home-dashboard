#include "tesla.h"

#include <WiFi.h>

#include "data_mutex.h"
#include "hubitat_client.h"

namespace {
constexpr unsigned long FETCH_INTERVAL_MS = 30UL * 1000UL;
// Edit both arrays for your own vehicles -- IDs come from Hubitat's Maker
// API device list, names are just the display labels this page shows.
constexpr const char *VEHICLE_IDS[TeslaManager::NUM_VEHICLES] = {"4258", "4259"};
constexpr const char *VEHICLE_NAMES[TeslaManager::NUM_VEHICLES] = {"Vehicle 1", "Vehicle 2"};
}  // namespace

void TeslaManager::loop() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    unsigned long now = millis();
    if (lastFetchMs != 0 && (now - lastFetchMs) < FETCH_INTERVAL_MS) {
        return;
    }
    lastFetchMs = now;
    fetchNow();
}

bool TeslaManager::fetchNow() {
    bool anyOk = false;
    for (int i = 0; i < NUM_VEHICLES; i++) {
        JsonDocument doc;
        if (!hubitatGetDevice(VEHICLE_IDS[i], doc)) {
            continue;
        }

        TeslaVehicleData v;
        v.name = VEHICLE_NAMES[i];
        v.batteryPercent = static_cast<int>(hubitatAttrFloat(doc, "battery", 0));
        v.insideTempF = hubitatAttrFloat(doc, "inside_temp", 0);
        v.outsideTempF = hubitatAttrFloat(doc, "outside_temp", 0);
        v.lockState = hubitatAttrString(doc, "lock");
        v.state = hubitatAttrString(doc, "state");
        v.present = hubitatAttrString(doc, "presence") == "present";
        v.valid = true;

        DataLock lock;
        current[i] = v;
        anyOk = true;
    }
    return anyOk;
}

TeslaVehicleData TeslaManager::data(int index) const {
    DataLock lock;
    if (index < 0 || index >= NUM_VEHICLES) {
        return TeslaVehicleData();
    }
    return current[index];
}
