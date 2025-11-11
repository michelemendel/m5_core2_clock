#ifndef CALENDAR_H
#define CALENDAR_H

#include <Arduino.h>
#include <time.h>

// Hebrew calendar calculations
bool isHebrewLeapYear(int year);
int getHebrewYearDays(int year);
long daysFromHebrewEpoch(int hYear);
int getHebrewMonthDays(int hYear, int hMonth);

// Gregorian calendar calculations
long gregorianToAbsolute(int year, int month, int day);

// Sunset calculation
int calculateSunsetHour(int year, int month, int day, int timezoneOffsetHours, bool dstOffset);

// Hebrew date conversion
void convertToHebrewDate(struct tm* gregorian, int* hebrewYear, int* hebrewMonth, int* hebrewDay, int timezoneOffsetHours, bool dstOffset);
String formatHebrewDate(struct tm* timeinfo, int timezoneOffsetHours, bool dstOffset);

#endif // CALENDAR_H

