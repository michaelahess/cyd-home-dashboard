#include "hubitat_client.h"

#include <HTTPClient.h>
#include <WiFi.h>

#include "secrets.h"

namespace {
String deviceUrl(const String &deviceId) {
    return String(HUBITAT_BASE_URL) + "/apps/api/" + HUBITAT_APP_ID + "/devices/" + deviceId;
}
}  // namespace

bool hubitatConfigured() {
    return HUBITAT_BASE_URL[0] != '\0';
}

bool hubitatGetDevice(const String &deviceId, JsonDocument &outDoc) {
    if (!hubitatConfigured() || WiFi.status() != WL_CONNECTED) {
        return false;
    }

    HTTPClient http;
    http.begin(deviceUrl(deviceId) + "?access_token=" + HUBITAT_TOKEN);
    int code = http.GET();
    if (code != 200) {
        Serial.printf("Hubitat device %s fetch failed, HTTP %d\n", deviceId.c_str(), code);
        http.end();
        return false;
    }
    String payload = http.getString();
    http.end();

    DeserializationError err = deserializeJson(outDoc, payload);
    if (err) {
        Serial.printf("Hubitat device %s parse failed: %s\n", deviceId.c_str(), err.c_str());
        return false;
    }
    return true;
}

String hubitatAttrString(JsonDocument &doc, const char *attrName) {
    for (JsonObject attr : doc["attributes"].as<JsonArray>()) {
        if (strcmp(attr["name"] | "", attrName) == 0) {
            if (attr["currentValue"].isNull()) {
                return "";
            }
            return attr["currentValue"].as<String>();
        }
    }
    return "";
}

float hubitatAttrFloat(JsonDocument &doc, const char *attrName, float defaultVal) {
    for (JsonObject attr : doc["attributes"].as<JsonArray>()) {
        if (strcmp(attr["name"] | "", attrName) == 0) {
            if (attr["currentValue"].isNull()) {
                return defaultVal;
            }
            return attr["currentValue"] | defaultVal;
        }
    }
    return defaultVal;
}

bool hubitatSendCommand(const String &deviceId, const String &command, const String &value) {
    if (!hubitatConfigured() || WiFi.status() != WL_CONNECTED) {
        return false;
    }

    String url = deviceUrl(deviceId) + "/" + command;
    if (value.length() > 0) {
        url += "/" + value;
    }
    url += "?access_token=" + String(HUBITAT_TOKEN);

    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    http.end();
    if (code != 200) {
        Serial.printf("Hubitat command %s on device %s failed, HTTP %d\n", command.c_str(), deviceId.c_str(), code);
        return false;
    }
    return true;
}
