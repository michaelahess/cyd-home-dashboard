#pragma once

#include <ArduinoJson.h>
#include <Arduino.h>

// Shared plumbing for the "POST a Jinja2 template to Home Assistant's
// /api/template, get one JSON object back" pattern used by every HA-backed
// data source in this project (tank temperatures, pump, Tesla, HVAC, window sensors). One request per call -- callers own their own refresh timing.
bool haFetchTemplate(const String &jinjaTemplate, JsonDocument &outDoc);

// POST to /api/services/<domain>/<service> with `body` as the JSON payload
// (entity_id plus whatever the service takes) -- how pages send commands,
// e.g. haCallService("climate", "set_hvac_mode", body). True on HTTP 200.
bool haCallService(const char *domain, const char *service, JsonDocument &body);

// True if HA_BASE_URL (secrets.h) is non-empty. Home Assistant is optional
// -- a user without it can leave HA_BASE_URL "" and the tank/pump pages
// simply won't fetch or appear.
bool haConfigured();
