#include <Arduino.h>
#include <ArduinoJson.h>

#include <PsychicHttp.h>
#include <PsychicHttpsServer.h>

#include <WiFi.h>

#include "configdata.h"

#include "htmldata.h"

#ifdef PSY_ENABLE_SSL
extern PsychicHttpsServer server;
#else
extern PsychicHttpServer server;
#endif

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

void doAccessPointWifiSetup() {
  Serial.println("Inside AP setup...");

  // todo: get a better name for this, to include the MAC
  WiFi.mode(WIFI_AP);
  WiFi.softAP(get_ap_ssid(), "homeauto");
  Serial.println(WiFi.softAPIP());
}

void doAccessPointSetup() {
  // set up the pages
  server.on("/favicon.ico", HTTP_GET, [](PsychicRequest *request) {
    Serial.println("Request for /favicon.ico");

    PsychicResponse response(request);

    response.setContentType("image/x-icon");
    response.setContent(FavIconPng, FavIconPng_len);

    return response.send();
  });

  server.on("/favicon.png", HTTP_GET, [](PsychicRequest *request) {
    Serial.println("Request for /favicon.png");

    PsychicResponse response(request);

    response.setContentType("image/png");
    response.setContent(FavIconPng, FavIconPng_len);

    return response.send();
  });

  server.on("/", HTTP_GET, [](PsychicRequest *request) {
    Serial.println("Request for /");

    PsychicResponse response(request);

    response.addHeader("Content-Encoding", "gzip");
    response.setContentType("text/html");
    response.setContent(ApConfigHtml, ApConfigHtml_len);

    return response.send();
  });

  server.on("/api/ap", HTTP_POST, [](PsychicRequest *request) {
    Serial.println("Request for /api/ap");

    //load our JSON request
    JsonDocument json;
    String body = request->body();
    DeserializationError err = deserializeJson(json, body);

    // json has our configuration stuff here
    if (!json.containsKey("ssid") || !json.containsKey("password")) {
      return request->reply(400, "application/json",
                            "{ \"error\": \"missing values\" }");
    }

    strncpy(configData.ssid, json["ssid"], SSID_SIZE);
    strncpy(configData.key, json["password"], KEY_SIZE);

    saveConfiguration();

    json.clear();

    json["ssid"] = configData.ssid;
    json["password"] = configData.key;

    //serialize and return
    String jsonBuffer;
    serializeJson(json, jsonBuffer);
    return request->reply(200, "application/json", jsonBuffer.c_str());
  });
}
