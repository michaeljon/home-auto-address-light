#ifndef _ADDRESSLIGHT_CALCULATIONS_H
#define _ADDRESSLIGHT_CALCULATIONS_H

// See
//  https://gml.noaa.gov/grad/solcalc/
//  https://gml.noaa.gov/grad/solcalc/table.php?lat=33.65461&lon=-117.56029&year=2024
//  https://edwilliams.org/sunrise_sunset_algorithm.htm

double calculateSunrise(int year, int month, int day, double lat, double lng,
                        int localOffset);

double calculateSunset(int year, int month, int day, double lat, double lng,
                       int localOffset);

#endif // _ADDRESSLIGHT_CALCULATIONS_H
