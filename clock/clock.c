#include <Arduino.h>
#include <ErriezDS3231.h>
#include <Wire.h>

// rtc
static ErriezDS3231 ds3231;

// essentially 00:00:00 / midnight
static const unsigned long startOfTime = 0 * 3600 + 0 * 60 + 0;

// essentially 23:59:59 / one minute before midnight
static const unsigned long endOfTime = 23 * 3600 + 59 * 60 + 59;

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
ICACHE_RAM_ATTR
#endif
void alarmHandler() { alarmInterrupt = true; }

void setAlarm() {
  Serial.println(F("Set Alarm 1 at 15 seconds of every minute"));

  // Program alarm 1
  // Alarm1EverySecond, Alarm1MatchSeconds, Alarm1MatchMinutes, Alarm1MatchHours, Alarm1MatchDay,
  // Alarm1MatchDate

  ds3231.setAlarm1(Alarm1MatchSeconds, 0, 0, 0, 15);
  ds3231.alarmInterruptEnable(Alarm1, true);
}

bool getAlarm() { return ds3231.getAlarmFlag(Alarm1); }

void clearAlarm() { // Clear alarm 1 interrupt
  ds3231.clearAlarmFlag(Alarm1);
}

void initializeAlarms() {
  Serial.println("Setting up the periodic timer");

  // Attach to INT0 interrupt falling edge
  pinMode(ALARM_TRIGGER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALARM_TRIGGER_PIN), alarmHandler,
                  FALLING);
}

void initializeClock() {
  // Initialize RTC
  while (!ds3231.begin()) {
    Serial.println(F("RTC not found"));
    delay(3000);
  }

  // Enable RTC clock
  if (!ds3231.isRunning()) {
    Serial.println(F("Clock reset"));
    ds3231.clockEnable();
  }

  ds3231.setSquareWave(SquareWaveDisable);
  ds3231.outputClockPinEnable(false);
  ds3231.alarmInterruptEnable(Alarm1, false);
  ds3231.alarmInterruptEnable(Alarm2, false);
}

void initializeTime(long gmtOffset_sec, int daylightOffset_sec,
                    const char *ntpServer) {
  struct tm timeinfo;

  // Init and get the time
  Serial.println("calling configTime()");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(100);

  Serial.println("calling getLocalTime()");
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    delay(3000);
  }

  Serial.printf("got time, and it's %s\n", asctime(&timeinfo));

  // set time on ds3231
  time_t timeSinceEpoch = mktime(&timeinfo);
  Serial.printf("time since epoch %ld\n", timeSinceEpoch);

  // rtc setup
  Serial.println("Setting time on RTC");
  ds3231.setEpoch(timeSinceEpoch);
}

bool timeIsInOnRange(unsigned long lightOnTime, unsigned long lightOffTime,
                     long gmtOffset_sec) {
  struct tm timeinfo;

  // get the current time of day from the struct
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    delay(3000);
  }

  unsigned long currentTime =
      timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;

  // Serial.printf("current time: %02d:%02d:%02d\n", timeinfo.tm_hour,
  //               timeinfo.tm_min, timeinfo.tm_sec);

  // Serial.printf("current time: %ld\n", currentTime);
  // Serial.printf("lightOnTime: %ld\n", lightOnTime);
  // Serial.printf("lightOffTime: %ld\n", lightOffTime);

  if (lightOffTime < lightOnTime) {
    // normal case, lights go on in the "evening", we check if the current time
    // is between the ON time and midnight-1 or between midnight and the OFF time
    return ((lightOnTime <= currentTime && currentTime <= endOfTime) ||
            (startOfTime <= currentTime && currentTime <= lightOffTime));
  } else {
    // inverted case, lights go on in the "morning"
    return (lightOnTime <= currentTime && currentTime <= lightOffTime);
  }
}
