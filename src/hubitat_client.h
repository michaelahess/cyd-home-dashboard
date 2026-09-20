#pragma once

#include <ArduinoJson.h>
#include <Arduino.h>

// Shared plumbing for Hubitat's Maker API (plain HTTP, local network only).
// Device info comes back as {..., "attributes": [{"name":..., "currentValue":...}, ...]},
// so callers pull specific attributes out of the parsed document by name.

// GETs a device's full info (attributes + capabilities + commands) into outDoc.
bool hubitatGetDevice(const String &deviceId, JsonDocument &outDoc);

// Finds the named attribute in a document already fetched via
// hubitatGetDevice() and returns its currentValue, or "" / defaultVal if
// the attribute is missing or its value is null.
String hubitatAttrString(JsonDocument &doc, const char *attrName);
float hubitatAttrFloat(JsonDocument &doc, const char *attrName, float defaultVal = 0.0f);

// Sends a device command, with an optional single argument (e.g.
// hubitatSendCommand("4000", "setHeatingSetpoint", "70")). Not currently
// called anywhere yet -- see the note in hvac_zones.h about holding off on
// wiring actual control until the touch UX for it is designed.
bool hubitatSendCommand(const String &deviceId, const String &command, const String &value = "");

// True if HUBITAT_BASE_URL (secrets.h) is non-empty. Hubitat is optional --
// a user without it can leave HUBITAT_BASE_URL "" and the Tesla/HVAC pages
// and window/door status simply won't fetch or appear.
bool hubitatConfigured();
