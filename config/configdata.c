#include <EEPROM.h>

#include "sun/calculations.h"

#define NUMBER_OF_DAYS 366

static volatile int yearComputed = 0;
static TRANSITION_TIME autoTimes[NUMBER_OF_DAYS];

static volatile bool configDataLoaded = false;
CONFIGDATA configData;

#define MAGIC 'c'

void ensureConfigData() {
  Serial.println("Ensuring config data is loaded");

  if (configDataLoaded == false) {
    Serial.println("Reading config data from EEPROM");

    // blow it all away
    if (EEPROM.readBytes(0, &configData, CONFIGDATA_SIZE) != CONFIGDATA_SIZE) {
      Serial.println("Didn't read the right size from EEPROM, resetting");

      configData.magic[0] = 0;
      configData.magic[1] = 0;
      configData.magic[2] = 0;
      configData.magic[3] = 0;
    }

    if (configData.magic[0] != 'M' || configData.magic[1] != 'J' ||
        configData.magic[1] != MAGIC) {
      Serial.println("Read invalid block from EEPROM, defaulting");

      configData.magic[0] = 'M';
      configData.magic[1] = 'J';
      configData.magic[2] = MAGIC;
      configData.magic[3] = 0;

      // we haven't stored anything yet, we're going to default a bit
      strncpy(configData.ssid, "HouseOnHill", SSID_SIZE);
      strncpy(configData.key, "Marvel2021", KEY_SIZE);

      configData.transitionOption = TRANSITION_OPTION::AUTO;
      configData.timezoneOffset = -480 * 60; // seconds
      configData.timezoneAdjustment = 3600;  // seconds

      configData.latitude = 33.654732;
      configData.longitude = -117.557831;

      // save this
      Serial.println("Writing default values to EEPROM");
      EEPROM.writeBytes(0, &configData, CONFIGDATA_SIZE);
      EEPROM.commit();

      Serial.println("Updated config data in EEPROM");
    }

    Serial.println("Config data loaded");
    configDataLoaded = true;
  }
}

static void computeTransitionTimes(int year) {
  time_t loctime;
  struct tm timeinfo, *loctimeinfo;

  // the tz adjustment is in seconds (for reasons), we need it in hours here
  int tzadjust = (int)(configData.timezoneOffset / 3600);

  Serial.printf("Latitude: %f\n", configData.latitude);
  Serial.printf("Longitude: %f\n", configData.longitude);

  for (int day = 0; day < NUMBER_OF_DAYS; day++) {
    bzero(&timeinfo, sizeof(struct tm));

    timeinfo.tm_isdst = -1; /* Allow mktime to determine DST setting. */
    timeinfo.tm_mon = 0;
    timeinfo.tm_mday = day + 1;
    timeinfo.tm_year = year - 1900;

    loctime = mktime(&timeinfo);
    loctimeinfo = localtime(&loctime);

    double on =
        calculateSunset(loctimeinfo->tm_year + 1900, loctimeinfo->tm_mon + 1,
                        loctimeinfo->tm_mday, configData.latitude,
                        configData.longitude, tzadjust);

    double off =
        calculateSunrise(loctimeinfo->tm_year + 1900, loctimeinfo->tm_mon + 1,
                         loctimeinfo->tm_mday, configData.latitude,
                         configData.longitude, tzadjust);

    double sunset_hr = fmod(24 + on, 24.0);
    double sunset_min = modf(fmod(24 + on, 24.0), &sunset_hr) * 60;
    autoTimes[day].on = (int)sunset_hr * 3600 + (int)sunset_min * 60;

    double sunrise_hr = fmod(24 + off, 24.0);
    double sunrise_min = modf(fmod(24 + off, 24.0), &sunrise_hr) * 60;
    autoTimes[day].off = (int)sunrise_hr * 3600 + (int)sunrise_min * 60;
  }
}

TRANSITION_TIME getTransitionTimeForDay(int year, int dayOfYear) {
  Serial.printf("Getting transition times for day %03d %03d\n", year,
                dayOfYear);

  // compute the times, this is a first-time or on-boot thing, or when we need
  // the transition and the year changed on us
  if (yearComputed != year) {
    computeTransitionTimes(year);

    // and indicate that we've done this
    yearComputed = year;
  }

  return autoTimes[dayOfYear];
}
