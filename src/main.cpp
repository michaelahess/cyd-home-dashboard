#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>

#include "beep.h"
#include "data_mutex.h"
#include "display_settings.h"
#include "ha_client.h"
#include "home_power.h"
#include "hubitat_client.h"
#include "hvac_zones.h"
#include "light_sensor.h"
#include "night_settings.h"
#include "pages/clock_weather_page.h"
#include "pages/forecast_page.h"
#include "pages/hvac_page.h"
#include "pages/page.h"
#include "pages/settings_page.h"
#include "pages/solar_page.h"
#include "pages/tesla_page.h"
#include "pages/uptime_page.h"
#include "status_led.h"
#include "tesla.h"
#include "touch.h"
#include "uptime_kuma.h"
#include "weather.h"
#include "windows_status.h"

namespace {
constexpr uint32_t LOOP_INTERVAL_MS = 20;
constexpr uint32_t NETWORK_TASK_STACK_BYTES = 12288;

TFT_eSPI tft;
TouchTap touch;
LightSensor lightSensor;
WeatherManager weatherManager;
HomePowerManager homePowerManager;
HvacZonesManager hvacZonesManager;
TeslaManager teslaManager;
UptimeKumaManager uptimeKumaManager;
WindowsStatusManager windowsStatusManager;
Beeper beeper;
StatusLed statusLed;

ClockWeatherPage clockWeatherPage;
ForecastPage forecastPage;
SolarPage solarPage;
TeslaPage teslaPage;
HvacPage hvacPage;
UptimePage uptimePage;
SettingsPage settingsPage;

// Built at boot from whichever integrations secrets.h has configured (see
// buildPageList()) -- a user who hasn't set up Hubitat or Home Assistant
// just doesn't get those pages instead of seeing broken/empty ones.
constexpr int MAX_PAGES = 7;
Page *pages[MAX_PAGES];
int numPages = 0;
int currentPage = 0;

// The BOOT button (GPIO0) doubles as a rotation toggle at runtime -- it's
// only needed held-down at power-on to force the flashing bootloader, so
// reading it as a plain button the rest of the time is safe. Rotation
// state itself lives in DisplaySettings (shared with the Settings page's
// on-screen "Rotate" button).
constexpr int BOOT_BTN_PIN = 0;
constexpr uint32_t BOOT_BTN_DEBOUNCE_MS = 50;

constexpr int BACKLIGHT_PIN = 21;
constexpr int BACKLIGHT_FULL_DUTY = 255;

bool isNightHour(int hour) {
    return hour >= 21 || hour < 6;
}

void switchPage(int newIndex) {
    currentPage = (newIndex + numPages) % numPages;
    pages[currentPage]->onShow();
}

enum class PageId { CLOCK, FORECAST, SOLAR, TESLA, HVAC, UPTIME, SETTINGS };

// The swipe/tap order. Reorder this list to change the order pages appear
// in -- that's the whole mechanism, no other code needs to change. Entries
// for an integration you haven't configured in secrets.h are skipped
// automatically (see buildPageList()), so it's safe to leave all seven
// listed regardless of which ones apply to you.
constexpr PageId PAGE_ORDER[] = {
    PageId::CLOCK, PageId::FORECAST, PageId::SOLAR, PageId::TESLA, PageId::HVAC, PageId::UPTIME, PageId::SETTINGS,
};

void buildPageList() {
    numPages = 0;
    for (PageId id : PAGE_ORDER) {
        switch (id) {
            case PageId::CLOCK:
                pages[numPages++] = &clockWeatherPage;
                break;
            case PageId::FORECAST:
                pages[numPages++] = &forecastPage;
                break;
            case PageId::SOLAR:
                if (haConfigured()) {
                    pages[numPages++] = &solarPage;
                }
                break;
            case PageId::TESLA:
                if (hubitatConfigured()) {
                    pages[numPages++] = &teslaPage;
                }
                break;
            case PageId::HVAC:
                if (hubitatConfigured()) {
                    pages[numPages++] = &hvacPage;
                }
                break;
            case PageId::UPTIME:
                if (UptimeKumaManager::configured()) {
                    pages[numPages++] = &uptimePage;
                }
                break;
            case PageId::SETTINGS:
                pages[numPages++] = &settingsPage;
                break;
        }
    }
}

// Runs on the other core so a slow HTTP fetch never blocks touch polling or
// drawing in loop() -- that blocking was exactly what made the UI briefly
// stop responding to touch on pages that had just fetched new data.
void networkTask(void * /*pvParameters*/) {
    // A network call fired from this task right at the instant Wi-Fi
    // finished connecting hit a real lwIP assertion once during testing
    // ("Required to lock TCPIP core functionality" in sys_untimeout) --
    // lwIP's own post-connect housekeeping isn't necessarily done the
    // instant the CONNECTED status/event fires. Waiting a couple seconds
    // past that transition before this task's first fetch avoids landing
    // in that window; it's a mitigation for a timing race, not a proven
    // fix, but the device auto-recovers via reboot if it ever recurs.
    constexpr uint32_t WIFI_SETTLE_MS = 2000;
    bool wasConnected = false;
    uint32_t connectedAtMs = 0;

    for (;;) {
        bool connected = (WiFi.status() == WL_CONNECTED);
        if (connected && !wasConnected) {
            connectedAtMs = millis();
        }
        wasConnected = connected;

        if (connected && (millis() - connectedAtMs) > WIFI_SETTLE_MS) {
            weatherManager.loop();
            homePowerManager.loop();
            hvacZonesManager.loop();
            teslaManager.loop();
            uptimeKumaManager.loop();
            windowsStatusManager.loop();
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
}  // namespace

void setup() {
    Serial.begin(115200);

    pinMode(BOOT_BTN_PIN, INPUT_PULLUP);
    bool flipped = DisplaySettings::flipped();

    tft.init();
    tft.setRotation(flipped ? 3 : 1);  // landscape; BOOT button / Settings page toggles this
    tft.invertDisplay(true);  // this panel batch renders colors inverted otherwise
    tft.fillScreen(TFT_BLACK);

    ledcAttach(BACKLIGHT_PIN, 20000, 8);
    ledcWrite(BACKLIGHT_PIN, BACKLIGHT_FULL_DUTY);

    touch.begin();
    if (!touch.isCalibrated()) {
        // First boot on this unit (or NVS was erased) -- run the same
        // calibration flow the Settings page's "Recalibrate Touch" button
        // uses, so a fresh board is usable out of the box without needing
        // a separate compile flag or reflash.
        touch.calibrate(tft);
    }

    lightSensor.begin();
    beeper.begin();
    statusLed.begin();

    clockWeatherPage.begin(tft, weatherManager, homePowerManager);
    forecastPage.begin(tft, weatherManager);
    solarPage.begin(tft, homePowerManager);
    teslaPage.begin(tft, teslaManager);
    hvacPage.begin(tft, hvacZonesManager, beeper, weatherManager);
    uptimePage.begin(tft, uptimeKumaManager);
    settingsPage.begin(tft, touch, beeper);

    buildPageList();
    pages[currentPage]->onShow();

    g_dataMutex = xSemaphoreCreateMutex();

    weatherManager.begin();
    uptimeKumaManager.begin();

    xTaskCreatePinnedToCore(networkTask, "network", NETWORK_TASK_STACK_BYTES, nullptr, 1, nullptr, 0);
}

void loop() {
    static uint32_t lastLoopMs = 0;

    uint32_t now = millis();
    if (now - lastLoopMs < LOOP_INTERVAL_MS) {
        return;
    }
    lastLoopMs = now;

    // BOOT button (active-low) toggles display rotation and reboots into
    // it. Edge-triggered off a simple debounce timer rather than every
    // loop iteration the pin reads LOW, since a single press is easily a
    // few hundred ms long at this poll rate.
    static bool lastBootBtnPressed = false;
    static uint32_t bootBtnChangedMs = 0;
    bool bootBtnPressed = (digitalRead(BOOT_BTN_PIN) == LOW);
    if (bootBtnPressed != lastBootBtnPressed) {
        bootBtnChangedMs = now;
        lastBootBtnPressed = bootBtnPressed;
    }
    if (bootBtnPressed && (now - bootBtnChangedMs) >= BOOT_BTN_DEBOUNCE_MS) {
        DisplaySettings::toggleAndReboot();  // never returns
    }

    lightSensor.loop();
    // Either trigger is enough: the fixed clock window is a safety net in
    // case the light sensor's threshold turns out unreliable (its raw
    // reading is still uncalibrated -- see light_sensor.h). Computed here
    // (not inside ClockWeatherPage) since it also drives backlight dimming
    // on board 2, and needs to keep updating even while a different page
    // is the one currently visible.
    bool night = lightSensor.isDark();
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
        night = night || isNightHour(timeinfo.tm_hour);
    }
    clockWeatherPage.setNightMode(night);
    float nightFraction = NightSettings::percent() / 100.0f;
    ledcWrite(BACKLIGHT_PIN, night ? static_cast<int>(nightFraction * BACKLIGHT_FULL_DUTY) : BACKLIGHT_FULL_DUTY);
    statusLed.setBrightness(night ? nightFraction : 1.0f);

    beeper.loop();

    // Priority order (highest first): something down > HVAC actively
    // running > window state > idle/off. Only one color can show at a
    // time, so this is a deliberate choice, not the only reasonable one --
    // easy to reorder if a different priority makes more sense in practice.
    StatusColor led = StatusColor::OFF;
    if (uptimeKumaManager.configLoaded() && uptimeKumaManager.downCount() > 0) {
        led = StatusColor::RED;
    } else if (hvacZonesManager.anyActivelyHeatingOrCooling()) {
        led = StatusColor::PURPLE;
    } else if (windowsStatusManager.hasSensors()) {
        led = windowsStatusManager.anyOpen() ? StatusColor::BLUE : StatusColor::GREEN;
    }
    statusLed.setColor(led);

    TouchEvent ev = touch.poll();
    if (ev.tapped) {
        constexpr int NAV_EDGE = 24;
        bool navAllowed = !pages[currentPage]->blocksPageNav();
        if (navAllowed && ev.y < NAV_EDGE) {
            switchPage(currentPage - 1);
        } else if (navAllowed && ev.y > 240 - NAV_EDGE) {
            switchPage(currentPage + 1);
        } else {
            pages[currentPage]->onTap(ev.x, ev.y);
        }
    }

    pages[currentPage]->loop();
}
