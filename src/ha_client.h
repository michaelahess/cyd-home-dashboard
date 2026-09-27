#pragma once

#include <ArduinoJson.h>
#include <Arduino.h>

// Shared plumbing for the "POST a Jinja2 template to Home Assistant's
// /api/template, get one JSON object back" pattern used by every HA-backed
// data source in this project (tank temperatures, auto-top-off pump). One request per call -- callers own their own refresh timing.
bool haFetchTemplate(const String &jinjaTemplate, JsonDocument &outDoc);

// True if HA_BASE_URL (secrets.h) is non-empty. Home Assistant is optional
// -- a user without it can leave HA_BASE_URL "" and the tank/pump pages
// simply won't fetch or appear.
bool haConfigured();
