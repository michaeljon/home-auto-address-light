#ifndef _ADDRESS_LIGHT_H
#define _ADDRESS_LIGHT_H

typedef enum { UNKNOWN, AP, STA } WIFI_MODE;

// Alarm interrupt flag must be volatile
extern volatile bool lightState;

// Alarm interrupt flag must be volatile
extern volatile bool manualLightState;

#define USER_BUTTON_PIN 16
#define ALARM_TRIGGER_PIN 23
#define LIGHT_DRIVER_PIN 32

#endif // _ADDRESS_LIGHT_H