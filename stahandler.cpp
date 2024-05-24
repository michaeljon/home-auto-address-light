#include <Arduino.h>
#include <ArduinoJson.h>

#ifdef ESP32
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#include "configdata.h"

// html data
#include "htmldata.h"

void notFound(AsyncWebServerRequest *request);

void doStationSetup(AsyncWebServer server) {
  Serial.println("Inside STA setup...");

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(configData.ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(configData.ssid, configData.key);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }
  WiFi.enableIpV6();

  Serial.println("");
  Serial.println("WiFi connected.");

  Serial.println(WiFi.localIP());

  // set up the pages
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Request for /");

    AsyncWebServerResponse *response = request->beginResponse_P(
        200, "image/x-icon", StaConfigHtml, StaConfigHtml_len);
    response->addHeader("Content-Type", "text/html");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/favicon.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Request for /favicon.png");

    AsyncWebServerResponse *response = request->beginResponse_P(
        200, "image/x-icon", FavIconPng, FavIconPng_len);
    request->send(response);
  });

  // set up the apis
  server.on("/api/configuration", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Request for /api/configuration");

    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    JsonDocument json;

    json["configurationType"] =
        configData.transitionOption == TRANSITION_OPTION::AUTO ? "auto"
                                                               : "manual";

    json["manualOn"] = configData.manualTime.on;
    json["manualOff"] = configData.manualTime.off;

    // the UI wants to see timezone offsets in minutes
    json["timezone"] = (int)(configData.timezoneOffset / 60);

    json["latitude"] = configData.latitude;
    json["longitude"] = configData.longitude;

    // serializeJson(json, *response);
    request->send(response);
  });

  server.on("/api/fw-version", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Request for /api/fw-version");

    AsyncWebServerResponse *response = request->beginResponse_P(
        200, "image/x-icon", FavIconPng, FavIconPng_len);

    JsonDocument json;
    json["version"] = "the version";
    json["manualOff"] = __DATE__;

    // serializeJson(json, *response);
    request->send(response);
  });

  server.onNotFound(notFound);
  server.begin();
}
