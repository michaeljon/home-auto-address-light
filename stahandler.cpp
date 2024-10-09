#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>

#include <PsychicHttp.h>
#include <PsychicHttpsServer.h>

#include <WiFi.h>

#include "configdata.h"

// html data
#include "htmldata.h"

#ifdef PSY_ENABLE_SSL
extern PsychicHttpsServer server;
#else
extern PsychicHttpServer server;
#endif

void doStationWifiSetup() {
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
  WiFi.enableIPv6();

  //set up our esp32 to listen on the local_hostname.local domain
  if (!MDNS.begin("addresslight")) {
    Serial.println("Error starting mDNS");
    return;
  }
  MDNS.addService("http", "tcp", 80);

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println(WiFi.localIP());
}

void doStationSetup() {
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

  // set up the apis
  server.on("/api/configuration", HTTP_GET, [](PsychicRequest *request) {
    Serial.println("Request for /api/configuration");

    PsychicResponse response(request);

    JsonDocument json;

    json["configurationType"] =
        configData.transitionOption == TransitionOption::Auto ? "auto"
                                                              : "manual";

    json["manualOn"] = configData.manualTimes.on;
    json["manualOff"] = configData.manualTimes.off;

    // the UI wants to see timezone offsets in minutes
    json["timezone"] = (int)(configData.timezoneOffset / 60);

    json["latitude"] = configData.latitude;
    json["longitude"] = configData.longitude;

    //serialize and return
    String jsonBuffer;
    serializeJson(json, jsonBuffer);
    return request->reply(200, "application/json", jsonBuffer.c_str());
  });

  server.on("/api/configuration", HTTP_POST, [](PsychicRequest *request) {
    Serial.println("Request for /api/configuration");

    //load our JSON request
    JsonDocument json;
    String body = request->body();
    DeserializationError err = deserializeJson(json, body);

    if (json.containsKey("configurationType")) {
      if (strcasecmp(json["configurationType"], "auto") == 0) {
        if (!json.containsKey("timezone") || !json.containsKey("latitude") ||
            !json.containsKey("longitude")) {
          return request->reply(
              400, "application/json",
              "{ \"error\": \"missing automatic configuration values\" }");
        }

        configData.timezoneOffset = atoi(json["timezone"]) * 60;

        configData.latitude = atof(json["latitude"]);
        configData.longitude = atof(json["longitude"]);

        configData.transitionOption = TransitionOption::Auto;
      } else if (strcasecmp(json["configurationType"], "manual") == 0) {
        if (!json.containsKey("manualOn") || !json.containsKey("manualOff")) {
          return request->reply(400, "application/json",
                                "{ \"error\": \"missing manual times\" }");
        }

        configData.transitionOption = TransitionOption::Manual;
      } else {
        return request->reply(400, "application/json",
                              "{ \"error\": \"invalid configurationType\" }");
      }
    }

    json.clear();

    json["configurationType"] =
        configData.transitionOption == TransitionOption::Auto ? "auto"
                                                              : "manual";

    json["manualOn"] = configData.manualTimes.on;
    json["manualOff"] = configData.manualTimes.off;

    // the UI wants to see timezone offsets in minutes
    json["timezone"] = (int)(configData.timezoneOffset / 60);

    json["latitude"] = configData.latitude;
    json["longitude"] = configData.longitude;

    //serialize and return
    String jsonBuffer;
    serializeJson(json, jsonBuffer);
    return request->reply(200, "application/json", jsonBuffer.c_str());
  });

  server.on("/api/fw-version", HTTP_GET, [](PsychicRequest *request) {
    Serial.println("Request for /api/fw-version");

    PsychicResponse response(request);

    JsonDocument json;
    json["version"] = "the version";
    json["manualOff"] = __DATE__;

    //serialize and return
    String jsonBuffer;
    serializeJson(json, jsonBuffer);
    return request->reply(200, "application/json", jsonBuffer.c_str());
  });

  // set up the pages
  server.on("/", HTTP_GET, [](PsychicRequest *request) {
    Serial.println("Request for /");

    PsychicResponse response(request);

    response.addHeader("Content-Encoding", "gzip");
    response.setContentType("text/html");
    response.setContent(StaConfigHtml, StaConfigHtml_len);

    return response.send();
  });
}
