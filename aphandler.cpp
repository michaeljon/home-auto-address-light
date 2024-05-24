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

#include "htmldata.h"

void notFound(AsyncWebServerRequest *request);

static char dec2hexchar(byte dec) {
  if (dec < 10)
    return '0' + dec;
  else
    return 'A' + (dec - 10);
}

static String get_ap_ssid() {
  static String ap_ssid;
  if (!ap_ssid.length()) {
    byte mac[6];
    WiFi.macAddress(mac);
    ap_ssid = "ALIGHT_";
    for (byte i = 3; i < 6; i++) {
      ap_ssid += dec2hexchar((mac[i] >> 4) & 0x0F);
      ap_ssid += dec2hexchar(mac[i] & 0x0F);
    }
  }
  return ap_ssid;
}

void doAccessPointSetup(AsyncWebServer server) {
  Serial.println("Inside AP setup...");

  // todo: get a better name for this, to include the MAC
  WiFi.mode(WIFI_AP);
  WiFi.softAP(get_ap_ssid(), "homeauto");
  Serial.println(WiFi.softAPIP());

  // set up the pages
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Request for /");

    AsyncWebServerResponse *response = request->beginResponse_P(
        200, "image/x-icon", ApConfigHtml, ApConfigHtml_len);
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
  server.onNotFound(notFound);
  server.begin();
}
