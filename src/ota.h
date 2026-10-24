#pragma once

// Over-the-air firmware updates (ArduinoOTA / espota, port 3232), so a clock
// can be reflashed over Wi-Fi instead of USB. Needs the two-slot partition
// table set in platformio.ini. Disabled (no listener at all) if OTA_PASSWORD
// in secrets.h is empty -- an unauthenticated flash port isn't acceptable.

// Call every loop(): starts the listener once Wi-Fi is up, then services it.
void otaLoop();

// True while an update is being received. The UI loop and network task skip
// their work so nothing competes with the flash write.
bool otaInProgress();
