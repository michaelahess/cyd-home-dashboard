#include "ha_client.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "secrets.h"

bool haConfigured() {
    return HA_BASE_URL[0] != '\0';
}

bool haFetchTemplate(const String &jinjaTemplate, JsonDocument &outDoc) {
    if (!haConfigured() || WiFi.status() != WL_CONNECTED) {
        return false;
    }

    JsonDocument reqDoc;
    reqDoc["template"] = jinjaTemplate;
    String body;
    serializeJson(reqDoc, body);

    WiFiClientSecure client;
    client.setInsecure();  // local HA instance; matches the family's existing dashboard config
    HTTPClient http;
    http.begin(client, String(HA_BASE_URL) + "/api/template");
    http.addHeader("Authorization", String("Bearer ") + HA_TOKEN);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(body);
    if (httpCode != 200) {
        Serial.printf("HA template fetch failed, HTTP %d\n", httpCode);
        http.end();
        return false;
    }
    String payload = http.getString();
    http.end();

    DeserializationError err = deserializeJson(outDoc, payload);
    if (err) {
        Serial.printf("HA template JSON parse failed: %s (raw: %s)\n", err.c_str(), payload.c_str());
        return false;
    }
    return true;
}
