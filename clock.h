#ifndef _ADDRESSLIGHT_CLOCK_H
#define _ADDRESSLIGHT_CLOCK_H

#include "address-light.h"

// Alarm interrupt flag must be volatile
extern volatile bool alarmInterrupt;

bool getAlarm();
void setAlarm();
void clearAlarm();

void initializeAlarms();
void initializeClock();

void initializeTime(char const *timezone, const char *ntpServer);
bool timeIsInOnRange(unsigned long lightOnTime, unsigned long lightOffTime,
                     long gmtOffset_sec);

#endif // _ADDRESSLIGHT_CLOCK_H
