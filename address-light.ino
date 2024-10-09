#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

#include <PsychicHttp.h>
#include <PsychicHttpsServer.h>

#include <WiFi.h>

#include <Wire.h>

// button
#include <InputDebounce.h>
#define BUTTON_DEBOUNCE_DELAY DEFAULT_INPUT_DEBOUNCE_DELAY // [ms]

// other stuff
#include <time.h>

#include "address-light.h"
#include "clock.h"
#include "configdata.h"

#ifdef PSY_ENABLE_SSL
#include "keys.h"
#endif
//
// web server and button handler
//
#ifdef PSY_ENABLE_SSL
PsychicHttpsServer server;
#else
PsychicHttpServer server;
#endif

InputDebounce toggleButton;

void doAccessPointWifiSetup();
void doAccessPointSetup();
void doStationWifiSetup();
void doStationSetup();

WIFI_MODE wifiMode = WIFI_MODE::UNKNOWN;

const char *ntpServer = "pool.ntp.org";

//
// lightState controls whether the light should be
// on or off, it's set by the SQW trigger on the DS3231
// and optionally when the on/off button is pressed
//
volatile bool lightState = false;

//
// manualLightState is set when the on/off button is
// pressed. doing so can turn the light on or off
// and will override the SQW trigger
//
volatile bool manualLightState = false;

// do nothing on button down
void toggleButton_pressedCallback(uint8_t pinIn) {}

// but, on button up, toggle the light
void toggleButton_releasedCallback(uint8_t pinIn) {
  Serial.println("Entering button handler");
  Serial.printf("manualLightState == %d\n", manualLightState);

  if (manualLightState == false) {
    // turn on the light
    lightState = true;
    manualLightState = true;
  } else {
    // turn off the light on the next pass
    lightState = false;
    manualLightState = false;
  }

  Serial.println("Leaving button handler");
  Serial.printf("manualLightState == %d\n", manualLightState);
  Serial.printf("lightState == %d\n", lightState);
}

//
// TODO: move these somewhere else
//
static char dec2hexchar(byte dec) {
  if (dec < 10)
    return '0' + dec;
  else
    return 'A' + (dec - 10);
}

static String get_ota_host() {
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

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  Wire.begin();
  Wire.setClock(400000);

  // need somewhere to read / write, so init here
  Serial.println("Initializing EEPROM");
  if (EEPROM.begin(CONFIGDATA_SIZE) == false) {
    Serial.println("Unable to initialize EEPROM");
    return;
  }

  ensureConfigData();

  // button stuff
  Serial.println("Setting the configuration button");
  pinMode(USER_BUTTON_PIN, INPUT);

  // set the pin mode so we can turn on / off the light
  Serial.println("Setting pin mode for light");
  pinMode(LIGHT_DRIVER_PIN, OUTPUT);

  // we're going to wait a few seconds after the boot
  // to give the user the chance to push the button
  // normally this would be a two-buttons-simultaneously
  Serial.println("Waiting for the user to press the button");
  delay(5 * 1000);

  // now, let's see if it's down, this'll tell us whether to
  // start in AP or STA mode
  Serial.println("Checking button state");
  if (digitalRead(USER_BUTTON_PIN) == LOW) {
    // debounce
    delay(BUTTON_DEBOUNCE_DELAY);
    if (digitalRead(USER_BUTTON_PIN) == LOW) {
      // start in AP mode
      wifiMode = WIFI_MODE::AP;
    } else {
      // start in STA mode (unless we don't have a config)
      wifiMode = WIFI_MODE::STA;
    }
  } else {
    // start in STA mode (unless we don't have a config)
    wifiMode = WIFI_MODE::STA;
  }

  Serial.println("Doing wifi setup...");

  if (wifiMode == WIFI_MODE::STA) {
    if (configData.ssid[0] == 0 || configData.key[0] == 0) {
      Serial.println("We don't have wifi settings, resorting to AP mode");
      wifiMode = WIFI_MODE::AP;
    }
  }

  if (wifiMode == WIFI_MODE::AP) {
    doAccessPointWifiSetup();
    Serial.println(F("Setup complete, running in ap mode..."));
  } else if (wifiMode == WIFI_MODE::STA) {
    doStationWifiSetup();
    Serial.println(F("Setup complete, running in station mode..."));
  } else {
    Serial.println("We don't have a wifi mode set");
    return;
  }

  Serial.println("Wifi configured, enabling OTA");
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname(get_ota_host().c_str());

  // this is !!Frogger0 hashed, for now
  ArduinoOTA.setPasswordHash("6997a0231010f0e1fc6008d6a77e3756");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  Serial.println("Configuring HTTP(S) server");

#ifdef PSY_ENABLE_SSL
  server.ssl_config.httpd.max_uri_handlers = 20;

  server.listen(443, server_cert, server_key);

  //this creates a 2nd server listening on port 80 and redirects all requests HTTPS
  PsychicHttpServer *redirectServer = new PsychicHttpServer();
  redirectServer->config.ctrl_port = 20424;
  redirectServer->listen(80);
  redirectServer->onNotFound([](PsychicRequest *request) {
    String url = "https://" + request->host() + request->url();
    return request->redirect(url.c_str());
  });

  server.listen(80);
#else
  server.listen(80);
#endif

  //setup server config stuff here
  server.config.max_uri_handlers = 20;
  server.config.stack_size = 8192;

  //example callback everytime a connection is opened
  server.onOpen([](PsychicClient *client) {
    Serial.printf("[http] connection #%u opened\n", client->socket());
  });

  //example callback everytime a connection is closed
  server.onClose([](PsychicClient *client) {
    Serial.printf("[http] connection #%u closed\n", client->socket());
  });

  //you can set up a custom 404 handler.
  server.onNotFound([](PsychicRequest *request) {
    Serial.printf("404 URL %s\n", request->url());
    Serial.printf("404 URI %s\n", request->uri());

    return request->reply(
        404, "text/html",
        "<html><body>Are you lookin' for somethin'?</body></html>");
  });

  if (wifiMode == WIFI_MODE::AP) {
    doAccessPointSetup();

    Serial.println(F("Setup complete, running in ap mode..."));
  } else if (wifiMode == WIFI_MODE::STA) {
    doStationSetup();

    // Setup the button that lets us toggle the light
    toggleButton.registerCallbacks(toggleButton_pressedCallback,
                                   toggleButton_releasedCallback);

    toggleButton.setup(USER_BUTTON_PIN, BUTTON_DEBOUNCE_DELAY,
                       InputDebounce::PIM_EXT_PULL_UP_RES);

    initializeClock();
    initializeTime(configData.timezoneOffset, configData.timezoneAdjustment,
                   ntpServer);

    initializeAlarms();
    setAlarm();

    Serial.println(F("Setup complete, running in station mode..."));
  } else {
    Serial.println("We don't have a wifi mode set");
    return;
  }
}

void handleAlarm() {
  if (alarmInterrupt == true) {
    Serial.printf("(enter) alarmInterrupt = %d\n", alarmInterrupt);

    if (getAlarm() == false) {
      Serial.printf("False alarm, returning");
      alarmInterrupt = false;
      return;
    }

    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      delay(3000);
    }

    bool _lightState = lightState;
    TransitionTimes transitionTime =
        getTransitionTimeForDay(timeinfo.tm_year + 1900, timeinfo.tm_yday);

    // check if the light should be on (after _ON_ but before _OFF_)
    if (timeIsInOnRange(transitionTime.on, transitionTime.off,
                        configData.timezoneAdjustment)) {
      // turn it on
      _lightState = true;
    } else {
      // turn it off
      _lightState = false;
    }

    Serial.printf("_lightState = %d\n", _lightState);

    // check if the light should physically change state
    if (lightState != _lightState) {
      if (_lightState == true) {
        Serial.println("Turning lights on");
      } else {
        Serial.println("Turning lights off");
      }
      lightState = _lightState;
    }

    Serial.printf("(exit) alarmInterrupt = %d\n", alarmInterrupt);

    clearAlarm();
  }
}

void handleLightState() {
  if (lightState == true) {
    // turn on the light
    digitalWrite(LIGHT_DRIVER_PIN, HIGH);
  } else {
    // turn off the light
    digitalWrite(LIGHT_DRIVER_PIN, LOW);
  }
}

void loop() {
  unsigned long now = millis();

  // see if there's an update pending
  ArduinoOTA.handle();

  toggleButton.process(now); // callbacks called in context of this function

  // only listen to alarms if we're not manually running
  if (manualLightState == false) {
    handleAlarm();

    // clear the alarm
    alarmInterrupt = false;
  }

  handleLightState();

  delay(5);
}
