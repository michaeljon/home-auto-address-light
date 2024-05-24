#include "calculations.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926
#endif

#define ZENITH -.83

typedef enum { Sunrise, Sunset } SunEvent_t;

#define DEGREES_TO_RADIANS(_d) (M_PI / 180 * _d)
#define RADIANS_TO_DEGREES(_r) (180 / M_PI * _r)

static double calculateSunEvent(int year, int month, int day, double lat,
                                double lng, int localOffset, SunEvent_t event) {
  // 1. first calculate the day of the year
  double N1 = floor(275 * month / 9);
  double N2 = floor((month + 9) / 12);
  double N3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3));
  double N = N1 - (N2 * N3) + day - 30;

  // 2. convert the longitude to hour value and calculate an approximate time
  double lngHour = lng / 15.0;
  double t;

  if (event == Sunrise) {
    t = N + ((6 - lngHour) / 24);
  } else {
    t = N + ((18 - lngHour) / 24);
  }

  // 3. calculate the Sun's mean anomaly
  double M = (0.9856 * t) - 3.289;

  // 4. calculate the Sun's true longitude
  double L = fmod(M + (1.916 * sin(DEGREES_TO_RADIANS(M))) +
                      (0.020 * sin(2 * DEGREES_TO_RADIANS(M))) + 282.634,
                  360.0);
  // 5a. calculate the Sun's right ascension
  double RA = fmod(
      RADIANS_TO_DEGREES(atan(0.91764 * tan(DEGREES_TO_RADIANS(L)))), 360.0);

  // 5b. right ascension value needs to be in the same quadrant as L
  double Lquadrant = floor(L / 90) * 90;
  double RAquadrant = floor(RA / 90) * 90;
  RA = RA + (Lquadrant - RAquadrant);

  // 5c. right ascension value needs to be converted into hours
  RA = RA / 15;

  // 6. calculate the Sun's declination
  double sinDec = 0.39782 * sin(DEGREES_TO_RADIANS(L));
  double cosDec = cos(asin(sinDec));

  // 7a. calculate the Sun's local hour angle
  double cosH = (sin(DEGREES_TO_RADIANS(ZENITH)) -
                 (sinDec * sin(DEGREES_TO_RADIANS(lat)))) /
                (cosDec * cos(DEGREES_TO_RADIANS(lat)));

  /*
    if (cosH >  1)
      the sun never rises on this location (on the specified date)
    if (cosH < -1)
      the sun never sets on this location (on the specified date)
  */

  // 7b. finish calculating H and convert into hours
  double H;
  if (event == Sunrise) {
    H = 360 - RADIANS_TO_DEGREES(acos(cosH)); //   if if rising time is desired:
  } else {
    H = RADIANS_TO_DEGREES(acos(cosH));
  }
  H = H / 15;

  // 8. calculate local mean time of rising/setting
  double T = H + RA - (0.06571 * t) - 6.622;

  // 9. adjust back to UTC
  double UT = fmod(T - lngHour, 24.0);

  // 10. convert UT value to local time zone of latitude/longitude
  return UT + localOffset;
}

double calculateSunrise(int year, int month, int day, double lat, double lng,
                        int localOffset) {
  return calculateSunEvent(year, month, day, lat, lng, localOffset,
                           SunEvent_t::Sunrise);
}

double calculateSunset(int year, int month, int day, double lat, double lng,
                       int localOffset) {
  return calculateSunEvent(year, month, day, lat, lng, localOffset,
                           SunEvent_t::Sunset);
}
