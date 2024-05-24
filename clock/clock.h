#ifndef _ADDRESSLIGHT_CLOCK_H
#define _ADDRESSLIGHT_CLOCK_H

#include "../address-light.h"

// Alarm interrupt flag must be volatile
volatile bool alarmInterrupt = false;

bool getAlarm();
void clearAlarm();

void initializeAlarms();
void initializeClock();

void initializeTime(long gmtOffset_sec, int daylightOffset_sec,
                    const char *ntpServer);

bool timeIsInOnRange(unsigned long lightOnTime, unsigned long lightOffTime,
                     long gmtOffset_sec);

#include "clock.c"

#endif // _ADDRESSLIGHT_CLOCK_H
