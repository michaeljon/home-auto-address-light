#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>

// button
#include <InputDebounce.h>

#define BUTTON_DEBOUNCE_DELAY DEFAULT_INPUT_DEBOUNCE_DELAY // [ms]

// other stuff
#include <time.h>

#include "address-light.h"
#include "clock/clock.h"
#include "config/configdata.h"

WIFI_MODE wifiMode = WIFI_MODE::UNKNOWN;

InputDebounce toggleButton;

const char *ntpServer = "pool.ntp.org";

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

void doAccessPointSetup() { Serial.println("Inside AP setup..."); }

// for now
const char *ssid = "HouseOnHill";
const char *password = "Marvel2021";

void doStationSetup() {
  Serial.println("Inside STA setup...");

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(configData.ssid);
  WiFi.begin(configData.ssid, configData.key);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  WiFi.enableIPv6();
  Serial.println("");
  Serial.println("WiFi connected.");

  Serial.println(WiFi.localIP());

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

  Serial.println("Running...");

  if (wifiMode == WIFI_MODE::AP) {
    // if we're in AP mode then we really don't do much
    // just setup soft AP mode and wire up the WIFI
    // config index.html
    doAccessPointSetup();
  } else if (wifiMode == WIFI_MODE::STA) {
    // otherwise we're in station mode, so wire up the
    // device configuration index.html
    doStationSetup();
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
    TRANSITION_TIME transitionTime =
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

    // clear the alarm
    alarmInterrupt = false;

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

  toggleButton.process(now); // callbacks called in context of this function

  // only listen to alarms if we're not manually running
  if (manualLightState == false) {
    handleAlarm();
  }

  handleLightState();

  delay(5);
}
