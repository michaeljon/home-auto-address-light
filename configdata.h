#ifndef _ADDRESSLIGHT_CONFIGDATA_H
#define _ADDRESSLIGHT_CONFIGDATA_H

#include <EEPROM.h>

#define SSID_SIZE 32
#define KEY_SIZE 32

typedef enum { AUTO, MANUAL } TRANSITION_OPTION;

typedef struct {
  unsigned long on;
  unsigned long off;
} TRANSITION_TIME;

typedef struct {
  char magic[4];

  // wifi settings
  char ssid[SSID_SIZE + 1];
  char key[KEY_SIZE + 1];

  // choice for controlling the light
  TRANSITION_OPTION transitionOption;

  // optional
  TRANSITION_TIME manualTime;

  // selected timezone offset (in seconds)
  short timezoneOffset;
  // seconds to add / subtract for DST (typically 3600 seconds)
  short timezoneAdjustment;

  // location
  double latitude;
  double longitude;
} CONFIGDATA;

#define CONFIGDATA_SIZE sizeof(CONFIGDATA)
#define MAGIC 'd'

void ensureConfigData();
TRANSITION_TIME getTransitionTimeForDay(int year, int dayOfYear);

extern CONFIGDATA configData;

#endif // _ADDRESSLIGHT_CONFIGDATA_H