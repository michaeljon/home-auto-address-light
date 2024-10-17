#include <Preferences.h>

#include "calculations.h"
#include "configdata.h"

#define NUMBER_OF_DAYS 366

static volatile int yearComputed = 0;
static TransitionTimes autoTimes[NUMBER_OF_DAYS];

CONFIGDATA configData;

Preferences nvs;

static void open() {
  // need somewhere to read / write, so init here
  Serial.println("Initializing preferences store");
  nvs.begin(PREFS_STORE_KEY, PREFS_MODE);
}

static void close() {
  // need somewhere to read / write, so init here
  Serial.println("Closing preferences store");
  nvs.end();
}

void initializeNewConfigData() {
  // create a new one
  memset(&configData, 0, CONFIGDATA_SIZE);
  configData.magic = MAGIC;

  // we haven't stored anything yet, we're going to default a bit
  memset(configData.ssid, 0, SSID_SIZE);
  strncpy(configData.ssid, "HouseOnHill", SSID_SIZE);
  memset(configData.key, 0, KEY_SIZE);
  strncpy(configData.key, "Marvel2021", KEY_SIZE);

  configData.transitionOption = TransitionOption::Auto;

  memset(configData.timezone, 0, TIMEZONE_SIZE);
  strncpy(configData.timezone, "PST8PDT,M3.2.0,M11.1.0", TIMEZONE_SIZE);

  configData.timezoneAdjustment = 3600; // seconds

  configData.latitude = 33.654732;
  configData.longitude = -117.557831;

  // ...and save it to the store
  Serial.println("writing new config data to preference cache...");
  nvs.putBytes(PREFS_ITEM_KEY, &configData, CONFIGDATA_SIZE);
}

void ensureConfigData() {
  Serial.println("Ensuring config data is loaded");
  open();

  memset(&configData, 0, CONFIGDATA_SIZE);

  if (nvs.isKey(PREFS_ITEM_KEY) == true) {
    Serial.println("Config data is present in preferences cache, loading...");

    // read it from the store
    nvs.getBytes(PREFS_ITEM_KEY, &configData, CONFIGDATA_SIZE);
  }

  if (configData.magic != MAGIC) {
    Serial.println("Missing or no config data in cache, (re)building...");
    initializeNewConfigData();
  }

  Serial.println("Config data loaded");

  close();
}

void printConfigData() {
  Serial.printf("configData\n");
  Serial.printf("    configData.ssid = %s\n", configData.ssid);
  Serial.printf("    configData.key = %s\n", configData.key);

  Serial.printf("    configData.transitionOption = %s\n",
                configData.transitionOption == TransitionOption::Auto
                    ? "auto"
                    : "manual");
  Serial.printf("    configData.manualTimes.on = %d\n",
                configData.manualTimes.on);
  Serial.printf("    configData.manualTimes.off = %d\n",
                configData.manualTimes.off);

  Serial.printf("    configData.timezoneOffset = %s\n", configData.timezone);
  Serial.printf("    configData.timezoneAdjustment = %d\n",
                configData.timezoneAdjustment);

  Serial.printf("    configData.latitude = %f\n", configData.latitude);
  Serial.printf("    configData.longitude = %f\n", configData.longitude);
}

void saveConfiguration() {
  open();

  nvs.putBytes(PREFS_ITEM_KEY, &configData, CONFIGDATA_SIZE);
  Serial.println("Updated config data in preferences store");

  close();
}

static void computeTransitionTimes(int year) {
  time_t loctime;
  struct tm timeinfo, *loctimeinfo;

  // this is a complete hack, reach into the timezone and grab the hours
  int tzAdjust = -7;

  Serial.printf("tzAdjust: %d\n", tzAdjust);
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

    double sunset =
        calculateSunset(loctimeinfo->tm_year + 1900, loctimeinfo->tm_mon + 1,
                        loctimeinfo->tm_mday, configData.latitude,
                        configData.longitude, tzAdjust);

    if (loctimeinfo->tm_isdst == 0) {
      sunset -= 1;
    }

    double sunrise =
        calculateSunrise(loctimeinfo->tm_year + 1900, loctimeinfo->tm_mon + 1,
                         loctimeinfo->tm_mday, configData.latitude,
                         configData.longitude, tzAdjust);

    if (loctimeinfo->tm_isdst == 0) {
      sunrise -= 1;
    }

    double sunset_hr = fmod(24 + sunset, 24.0);
    double sunset_min = modf(fmod(24 + sunset, 24.0), &sunset_hr) * 60;
    autoTimes[day].on = (int)sunset_hr * 3600 + (int)sunset_min * 60;

    double sunrise_hr = fmod(24 + sunrise, 24.0);
    double sunrise_min = modf(fmod(24 + sunrise, 24.0), &sunrise_hr) * 60;
    autoTimes[day].off = (int)sunrise_hr * 3600 + (int)sunrise_min * 60;

    if ((day >= 65 && day <= 75) || (day >= 300 && day <= 315)) {
      Serial.printf(
          "[%03d] %04d-%02d-%02d: %ld-%ld %02d:%02d  %02d:%02d  %d %s\n", day,
          loctimeinfo->tm_year + 1900, loctimeinfo->tm_mon + 1,
          loctimeinfo->tm_mday, autoTimes[day].on, autoTimes[day].off,
          (int)sunset_hr, (int)sunset_min, (int)sunrise_hr, (int)sunrise_min,
          loctimeinfo->tm_isdst, *tzname);
    }
  }
}

TransitionTimes getTransitionTimeForDay(int year, int dayOfYear) {
  Serial.printf("Getting transition times for day %03d %03d\n", year,
                dayOfYear);

  // compute the times, this is a first-time or on-boot thing, or when we need
  // the transition and the year changed on us
  if (yearComputed != year) {
    Serial.println("No times present in RAM, computing them now.");
    computeTransitionTimes(year);

    // and indicate that we've done this
    yearComputed = year;
  }

  Serial.printf("day: %d  on: %ld  off: %ld\n", dayOfYear,
                autoTimes[dayOfYear].on, autoTimes[dayOfYear].off);
  return autoTimes[dayOfYear];
}
