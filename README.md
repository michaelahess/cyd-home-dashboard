# ESP32 CYD Home Dashboard

Turns a CYD ("Cheap Yellow Display", ESP32-2432S028R) into a home dashboard
on its built-in 320x240 TFT. One firmware image, no per-board build flags --
first boot on a fresh unit walks you through touch calibration on-screen,
and everything else (which pages appear, your location, dimming level, beep
volume, display orientation) is either a `secrets.h` setting or adjustable
live from the on-device Settings page. Tap the top of the screen to go to
the previous page, tap the bottom for the next page. Buttons on the HVAC and
Settings pages give a short click on the onboard speaker pad, and HVAC mode
changes (off/heat/cool/auto) get a distinct two-note chime instead, since
those actuate a real unit. The onboard RGB LED doubles as an at-a-glance
status light (see "Status LED" below).

Pages, in default order (some are skipped entirely if you haven't
configured that integration -- see Setup):

1. **Clock & weather** — large clock, date, and current local weather.
   Always present.
2. **5-Day Forecast** — one row per day with a hand-drawn condition icon,
   hi/lo temps, and precipitation chance. Always present.
3. **Tank Temps (1 of 2)** and 4. **Tank Temps (2 of 2)** — probe
   temperature for each tank, split across two pages (`NUM_TANKS` tanks,
   names and HA entities from `secrets.h`). Only shown if `HA_BASE_URL` is set.
5. **Pump** — auto-top-off pump: runs today, last/next run time, and
   reservoir wet/dry status (HA entities from `secrets.h`). Only shown if
   `HA_BASE_URL` is set.
6. **Tesla** — per-vehicle battery %, inside/outside temp, lock state, and
   online status (via Hubitat's TeslaMate-driven devices). Only shown if
   `HUBITAT_BASE_URL` is set.
7. **HVAC** — each zone's mode/temp/setpoint. Tap a zone row to open a
   control screen for it (mode, setpoint +/-, fan-mode cycle), with a Back
   button to return to the list. This actuates real thermostat units via
   Hubitat's Maker API — there's no confirmation step before a button press
   takes effect. Only shown if `HUBITAT_BASE_URL` is set.
8. **System Status** — Uptime Kuma up/down count, with a happy checkmark
   when everything's up or a list of what's down. Only shown if
   `UPTIME_KUMA_BASE_URL` is set.
9. **Settings** — recalibrate touch, flip the display 180 degrees, and
   adjust night-mode dimming level and beep volume, all live, all persisted
   to flash (NVS). Always present.

**To change the order**, edit the `PAGE_ORDER` array near the top of
`src/main.cpp` and reflash -- it's a plain list of page IDs read
top-to-bottom, so reordering is just reordering that list. An entry for an
integration you haven't configured is skipped automatically, so it's fine
to leave all nine listed regardless of which ones apply to you.

Board confirmed via `esptool`: ESP32-D0WD-V3, 4MB flash, CH340 USB-serial
bridge. 2.8", resistive-touch, dual-USB-port CYD variant. Developed and
tested across two physical units of this variant; other CYD variants (different
screen size/driver chip) would need `platformio.ini`'s TFT_eSPI build flags
adjusted.

Auto-dim: night mode (dim red text and weather icons, a dimmed backlight, and
a dimmed status LED) triggers from a fixed 9pm-6am clock window, OR'd with
the onboard light sensor if it's giving plausible readings (see the LDR note
below -- on the currently-tested boards it reads a flat, untrustworthy 0).
The dimming level (default 12%, confirmed comfortable via on-device testing
across two boards) is adjustable from the Settings page.

## Status LED

The onboard RGB LED reflects one status at a time (highest priority first),
all sourced from Hubitat/Uptime Kuma:

1. **Red** -- something in Uptime Kuma is down.
2. **Purple** -- any HVAC zone is actively heating or cooling right now.
3. **Blue** -- a window or door is open.
4. **Green** -- all windows/doors are closed and everything else is fine.

It dims along with the backlight at night (same trigger, see Auto-dim above).

## Touch calibration

Resistive touch panels vary enough unit-to-unit that no compiled-in
calibration fits every board. On first boot (or after erasing NVS), the
device automatically shows a 5-point on-screen calibration -- tap each
crosshair as it appears. The result is saved to flash, so this only happens
once per unit. To redo it later (e.g. the touch feels off, or you're
handing the board to someone else), use "Recalibrate Touch" on the Settings
page -- no reflash needed.

## Rotating the display

Press the physical BOOT button (GPIO0, the same button used to force
flashing mode -- reading it as a plain button at runtime is safe, it's only
needed held down at power-on) to flip the display 180 degrees, or use
"Rotate Display 180" on the Settings page -- both do the same thing. The
choice is saved to flash (NVS) and applied by rebooting into it, since this
board boots in a couple seconds -- simpler and more reliable than
re-flowing every page's layout live. Touch coordinates are mirrored to
match automatically.

## Settings page

The last page in the swipe/tap order. Everything here is live and persisted
to flash immediately, no reflash required:

- **Recalibrate Touch** -- reruns the 5-point calibration (see above).
- **Rotate Display 180** -- same as the physical BOOT button.
- **Night Dimming** -- the backlight/status-LED brightness percentage used
  at night (3-40%, step 3%).
- **Beep Volume** -- 0 (mute) to 100%, step 5%. Different physical
  boards/speakers are louder or quieter than the one this was tuned
  against, so this is a knob instead of a hardcoded constant.

## Setup

1. Copy `src/secrets.h.example` to `src/secrets.h` and fill in:
   - Wi-Fi SSID/password (required)
   - Either a ZIP/postal code (+ country, for outside the US) or a direct
     latitude/longitude, plus an IANA timezone name (see the comments in
     `secrets.h.example` for both options) (required)
   - A POSIX TZ string for `configTzTime()` (see the tz database) (required)
   - Optionally: a Home Assistant base URL + long-lived access token
     (Profile > Security > Long-Lived Access Tokens in HA) — powers the
     Tank Temps and Pump pages. Leave blank to skip.
   - Optionally: a Hubitat Maker API base URL/app id/token (create a "Maker
     API" app in Hubitat, expose whichever devices you want it to read) —
     powers the Tesla, HVAC, and window/door status. Leave blank to skip.
   - Optionally: a public Uptime Kuma status page base URL + slug — powers
     the System Status page. Leave blank to skip.
2. `secrets.h` is git-ignored and never committed -- so are the device IDs
   and display names/labels you put in it (see below), which matters if you
   ever publish your own fork, since those are inherently specific to (and
   can reveal details about) your own home setup.
3. *Which* Hubitat devices each page reads, and what to call them, are also
   `secrets.h` variables: `HVAC_ZONE_IDS`/`HVAC_ZONE_LABELS`,
   `TESLA_VEHICLE_IDS`/`TESLA_VEHICLE_NAMES`, and
   `NUM_WINDOW_SENSORS`/`WINDOW_SENSOR_IDS` (see the comments in
   `secrets.h.example`). Find device IDs via the Maker API app's device
   list or its `/devices` endpoint.
4. *Which* Home Assistant entities the Tank Temps and Pump pages read are
   `secrets.h` variables too: `TANK_ENTITY_IDS`/`TANK_LABELS` (exactly
   `NUM_TANKS` entries, see `tank_temps.h`) and `PUMP_RUNS_COUNTER`,
   `PUMP_LAST_RUN`, `PUMP_TIMER`, `PUMP_WET_SENSOR`.

## Build & flash

```
pio run
pio run -t upload
pio device monitor
```

`platformio.ini` is pinned to `/dev/cu.usbserial-10`. If the board enumerates on
a different port, override with `--upload-port` / `--monitor-port`, or edit
`upload_port` / `monitor_port` in `platformio.ini`.

## Home Assistant integration (tanks + pump)

A single POST to HA's `/api/template` endpoint with a small Jinja2 template
that renders several entity values as one JSON blob in a single request
(`src/ha_client.{h,cpp}`). The templates are built at runtime from the
entity IDs in `secrets.h`. Check Home Assistant's `/api/states` for
entities that are actively updating.

## Hubitat integration (Tesla + HVAC)

Uses Hubitat's Maker API (plain HTTP, local network only —
`src/hubitat_client.{h,cpp}`). Confirmed via that API's device list:

- **Tesla** (`src/tesla.{h,cpp}`): per-vehicle devices, IDs/names set via
  `TESLA_VEHICLE_IDS`/`TESLA_VEHICLE_NAMES` in `secrets.h`, type "TeslaMate
  Vehicle" — real per-car `battery`, `inside_temp`, `outside_temp`, `lock`,
  `state`, `presence` attributes.
- **HVAC** (`src/hvac_zones.{h,cpp}`): zone devices, IDs/labels set via
  `HVAC_ZONE_IDS`/`HVAC_ZONE_LABELS` in `secrets.h`, type "Mitsubishi Heat
  Pump MQTT" — real Hubitat Thermostat-capability devices with documented
  commands (`heat`/`cool`/`off`/`auto`,
  `setHeatingSetpoint`, `setCoolingSetpoint`, `fanAuto`/`fanOn`/`fanCirculate`).
  The device already bridges to the physical units via an existing
  Mosquitto MQTT link — this project only talks to Hubitat's HTTP API, never
  MQTT directly. Also reads `thermostatOperatingState` (heating/cooling/idle
  — whether a unit is actually running right now, not just its configured
  mode) to drive the status LED's purple state.
- **Windows/doors** (`src/windows_status.{h,cpp}`): standard
  ContactSensor-capability devices, IDs set via `NUM_WINDOW_SENSORS`/
  `WINDOW_SENSOR_IDS` in `secrets.h`, `contact` attribute is "open"/"closed"
  — drives the status LED's blue/green state.

## Hardware pinout reference (ESP32-2432S028R / CYD, 2.8" resistive)

Used by this project:

| Function | GPIO |
|---|---|
| TFT MOSI | 13 |
| TFT MISO | 12 |
| TFT SCLK | 14 |
| TFT CS | 15 |
| TFT DC | 2 |
| TFT RST | none (software reset) |
| TFT Backlight | 21 (PWM via LEDC, see Auto-dim above) |
| Touch (XPT2046) MOSI | 32 |
| Touch MISO | 39 |
| Touch CLK | 25 |
| Touch CS | 33 |
| Touch IRQ | 36 |
| BOOT button (rotation toggle) | 0 (active-low, internal pull-up) |
| RGB LED Red | 4 (active-low, PWM via LEDC -- notably dimmer than green/blue, see Known CYD quirks) |
| RGB LED Green | 17 (active-low, PWM via LEDC) |
| RGB LED Blue | 16 (active-low, PWM via LEDC) |
| Onboard speaker pad (2-pin JST, labeled "SPEAK") | 26 (PWM square-wave beeps/chimes) |

Also read, though not usefully yet (see Known CYD quirks):

| Function | GPIO |
|---|---|
| Light sensor (LDR, silkscreened "R21") | 34 (analog) -- currently reads a flat 0 on the tested board, see Known CYD quirks |

Not used by this project, kept here for reference:

| Function | GPIO |
|---|---|
| I2C header (CN1, 4-pin JST) | SCL=22, SDA=27, plus GND/3.3V |
| GPIO breakout header (P7, 4-pin JST) | IO35, IO22, IO21 (IO21 is shared with the backlight -- see caution below), plus GND |
| UART breakout (P5, 4-pin JST) | VIN/TX/RX/GND -- alternate serial path to the CH340 USB port |
| MicroSD slot ("TF") | present, unused by this project |

Other things visible on the board but not independently verified or used:
a small chip (U5) positioned right next to the SPEAK connector, which looks
like it could be a small audio amp for that pad rather than a bare DAC
line -- worth testing with an actual speaker before assuming it needs
external amplification. A few unlabeled round pads near the SD slot may be
unpopulated button footprints; not confirmed.

**Caution:** P7 breaks out IO21, which internally is also the TFT
backlight control pin. Driving that header pin externally would fight
whatever the backlight is doing.

## Known CYD quirks (confirmed on this board)

- **Colors are inverted by default on this panel batch.** `main.cpp` calls
  `tft.invertDisplay(true)` after `tft.init()` — without it, the background
  renders white instead of black and colors show as their complement (e.g.
  cyan renders as red). If you swap in a different panel and colors look
  wrong, try removing/toggling this line first.
- Display orientation (`tft.setRotation(1)` vs `(3)`) is runtime-toggled via
  the physical BOOT button or the Settings page rather than hardcoded --
  see "Rotating the display" above. Touch calibration is captured live
  under whichever rotation is active at the time (see "Touch calibration"),
  so rotating invalidates and re-triggers it rather than needing any
  separate coordinate-mirroring logic.
- **The onboard RGB LED's GPIO-to-color mapping doesn't match what's
  commonly documented for this board model.** Web research (never
  independently verified until a live per-GPIO test) suggested
  GPIO4/16/17 = red/green/blue. A "blue" status showing as green in
  practice led to testing each GPIO alone: GPIO16 actually produces blue
  and GPIO17 actually produces green -- swapped from the assumption, fixed
  in `status_led.cpp`. GPIO4 (red) looked wrong too in that same test (a
  blue/green blend instead of red), but a follow-up isolated blink test
  confirmed it does drive a real red channel -- it's just noticeably dimmer
  than green/blue at the same PWM duty, likely a real difference in the LED
  package's red die/current-limiting rather than a wiring problem (full
  duty is already maximum current, so there's no software fix to brighten
  it further). If you build this on your own board, don't assume this
  project's pin numbers are correct for yours either -- verify the same way.
- **Backlight PWM dimming: board 1 originally corrupted, board 2 didn't --
  then a retest on board 1 came back clean.** First round: any `ledcWrite`
  duty below 100% corrupted board 1's display (stuck white screen, sometimes
  recovering after several seconds), reproduced at two different PWM
  frequencies, while board 2 dimmed cleanly at the same settings and down to
  ~12% duty. Backlight PWM was gated behind the `CYD_BOARD_2` build flag for
  a while as a result. A later direct retest on board 1 (same pin, same
  ~12% duty) came back clean with no corruption, so PWM backlight dimming is
  now unconditional in `main.cpp` (`BACKLIGHT_PIN`/`BACKLIGHT_FULL_DUTY`,
  duty level from `src/night_settings.{h,cpp}`) for both boards. Neither
  result was ever explained (loose connection? warm-up state? never
  determined) -- if dimming ever misbehaves on a given unit, the Settings
  page's Night Dimming control can be set to a higher percentage (dimmer
  than 100% but well above the corrupting range) as a workaround, or the
  logic could be re-gated per-board if a real pattern emerges.
- **The onboard light sensor (LDR, GPIO34) reads a flat 0 on the
  board tested so far**, confirmed present on the PCB (silkscreened "R21",
  drawn as a variable resistor) but not producing a plausible varying
  reading from this pin. `light_sensor.cpp` treats an exact 0 as an
  invalid/disconnected reading (not genuine darkness) specifically so a
  stuck sensor can't force night-mode/dimming on permanently -- confirmed
  this mattered: without that guard, the flat 0 kept the display
  dimmed/night-colored all day. Until this is resolved (wrong pin for this
  variant? needs a pull resistor? genuinely unpopulated on this unit?), the
  fixed 9pm-6am clock window is the only reliable night-mode trigger. The
  board also breaks out an I2C header (CN1: GND/IO22/IO27/3.3V) that could
  carry an external light sensor if the onboard one can't be made to work.
- **Touch is not factory-calibrated and axes are NOT swapped**, confirmed via
  an on-device 5-point calibration: raw X tracks screen X and raw Y tracks
  screen Y directly (an earlier swapped-axis assumption, inferred rather
  than measured, caused real touch-target misfires before this was
  confirmed). Since the two boards this project was developed on measured
  meaningfully different raw ADC bounds, calibration now lives in
  `src/touch.{h,cpp}` as a per-unit NVS value rather than a compiled-in
  constant -- see "Touch calibration" above.
- **Network calls must not run on the same task as touch/display.** A
  blocking HTTP fetch on the main loop made the UI stop responding to touch
  until the fetch finished. Fixed by running all fetches on a background
  FreeRTOS task (`networkTask` in `main.cpp`) pinned to the other core, with
  a shared mutex (`src/data_mutex.{h,cpp}`) guarding each manager's cached
  data between that task and the render loop.
- **A network call fired right as Wi-Fi finished connecting hit a real lwIP
  assertion once** (`sys_untimeout`, "Required to lock TCPIP core
  functionality"). Mitigated by having the background task wait ~2s after
  the Wi-Fi-connected transition before its first fetch (see the comment in
  `networkTask`) -- a timing mitigation, not a proven fix, though the device
  auto-recovers via reboot if it recurs.

## Architecture notes

- `src/main.cpp` — board init, the background network task, touch-to-page
  dispatch (top/bottom edge strips page-navigate; taps in between forward to
  the current page's `onTap(x, y)`), night-mode/backlight/LED-brightness
  computation, `buildPageList()` (which pages exist, based on what's
  configured), first-boot calibration trigger, and the BOOT-button rotation
  toggle.
- `src/touch.{h,cpp}` — XPT2046 touch driver on its own VSPI bus (separate
  from the display's bus, per this board's wiring). Turns a tap into a
  calibrated screen (x, y), mirrored on both axes when the display is
  rotated 180 degrees. Owns the on-screen 5-point calibration flow
  (`calibrate()`) and its NVS-backed bounds, so one firmware image adapts to
  any physical unit instead of needing per-board compiled-in constants.
- `src/night_settings.{h,cpp}` — the persisted (NVS) night-dimming
  percentage shared by main.cpp's backlight/LED logic and the Settings page.
- `src/display_settings.{h,cpp}` — the persisted (NVS) display-rotation
  flag shared by the BOOT button and the Settings page.
- `src/light_sensor.{h,cpp}` — reads the onboard LDR; guards against a
  stuck 0 reading being treated as darkness (see Known CYD quirks above).
- `src/beep.{h,cpp}` — non-blocking LEDC square-wave tones on the speaker
  pad: a quiet click (`beep()`) for most taps, a two-note chime
  (`playHappyTone()`) for HVAC mode changes specifically. Volume is a
  persisted (NVS) percentage, adjustable from the Settings page.
- `src/status_led.{h,cpp}` — the onboard RGB LED, driven via LEDC PWM per
  channel (active-low) so both color and brightness are controllable; see
  "Status LED" above.
- `src/data_mutex.{h,cpp}` — the mutex shared by every manager below.
- `src/weather.{h,cpp}` — also resolves `ZIP_CODE`/`ZIP_COUNTRY` (secrets.h)
  to a lat/lon via a free zip-lookup API if one was given, otherwise uses
  `LATITUDE`/`LONGITUDE` directly.
- `src/tank_temps.{h,cpp}`, `src/fish_pump.{h,cpp}`, `src/hvac_zones.{h,cpp}`, `src/tesla.{h,cpp}`,
  `src/uptime_kuma.{h,cpp}`, `src/windows_status.{h,cpp}` — one manager per
  data source; each owns its own fetch timing and a `data()` accessor that
  returns a locked copy, safe to call from the render loop. Each no-ops
  (never makes a network call) when its integration isn't configured in
  `secrets.h`.
- `src/ha_client.{h,cpp}` / `src/hubitat_client.{h,cpp}` — shared HTTP
  plumbing for the two backends multiple managers talk to, plus
  `haConfigured()`/`hubitatConfigured()` helpers used to decide which pages
  `buildPageList()` includes.
- `src/pages/page.h` — the `onShow()`/`loop()`/`onTap(x,y)` interface every
  page implements.
- `src/pages/*.{h,cpp}` — one file pair per page (see the page list above).
  Pages that just display polled data cache the last-drawn values and only
  redraw when something actually changed, to avoid flicker.
- `src/pages/settings_page.{h,cpp}` — the on-device Settings page; see
  "Settings page" above.
- `src/pages/weather_icon.{h,cpp}` — shared hand-drawn condition icon
  (sun/cloud/rain/snow/storm), scalable, used by the clock and forecast
  pages. Takes an optional monochrome override color, used by the clock page
  at night so the icon reads as the same dim red as the surrounding text
  instead of its normal yellow/grey/cyan palette.
