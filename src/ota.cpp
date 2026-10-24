#include "ota.h"

#include <ArduinoOTA.h>
#include <WiFi.h>

#include "secrets.h"

namespace {
volatile bool updating = false;
bool started = false;

#if defined(CYD_PROFILE_SOLAR)
constexpr const char *HOSTNAME = "cyd-solar";
#else
constexpr const char *HOSTNAME = "cyd-fish";
#endif
}  // namespace

bool otaInProgress() {
    return updating;
}

void otaLoop() {
    if (OTA_PASSWORD[0] == '\0') {
        return;
    }
    if (!started) {
        if (WiFi.status() != WL_CONNECTED) {
            return;
        }
        ArduinoOTA.setHostname(HOSTNAME);
        ArduinoOTA.setPassword(OTA_PASSWORD);
        ArduinoOTA.onStart([]() {
            updating = true;
            Serial.println("OTA update starting");
        });
        ArduinoOTA.onEnd([]() { Serial.println("OTA update done, rebooting"); });
        ArduinoOTA.onError([](ota_error_t err) {
            Serial.printf("OTA error %u\n", static_cast<unsigned>(err));
            updating = false;  // resume normal operation after a failed/aborted update
        });
        ArduinoOTA.begin();
        started = true;
        Serial.printf("OTA ready as %s.local\n", HOSTNAME);
    }
    ArduinoOTA.handle();
}
