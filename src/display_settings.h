#pragma once

// Persisted (NVS) display rotation state -- shared by the physical BOOT
// button (main.cpp) and the Settings page's on-screen "Rotate" button.
// Applying a new value is done by rebooting into it rather than trying to
// re-flow every page's layout live; this board boots in a couple seconds,
// so that's a fine tradeoff.
namespace DisplaySettings {
bool flipped();          // loads from NVS on first call, cached after
void toggleAndReboot();  // flips the setting, saves it, then ESP.restart()s -- never returns
}  // namespace DisplaySettings
