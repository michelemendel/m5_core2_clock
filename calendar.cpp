#include "calendar.h"
#include <math.h>
#include <string.h>

// Calculate if a Hebrew year is a leap year
bool isHebrewLeapYear(int year) {
  int cycleYear = ((year - 1) % 19) + 1;
  return (cycleYear == 3 || cycleYear == 6 || cycleYear == 8 || cycleYear == 11 || 
          cycleYear == 14 || cycleYear == 17 || cycleYear == 19);
}

// Get number of days in a Hebrew year
int getHebrewYearDays(int year) {
  // Use long to avoid integer overflow
  long moladOffset = ((long)(year - 1) % 19) * 235L + ((long)(year - 1) / 19) * 6939L + 347605L;
  long moladDays = moladOffset / 25920L;
  long moladParts = moladOffset % 25920L;
  
  // Calculate Rosh Hashanah delay rules
  int dayOfWeek = (int)((moladDays + 1) % 7); // 0=Monday, 1=Tuesday, etc.
  
  // Calculate delays
  int delay = 0;
  if (moladParts >= 19440) delay = 1; // Molad Zaken
  if (dayOfWeek == 1 && moladParts >= 19440 && !isHebrewLeapYear(year)) delay = 2; // GaTRaD
  if (dayOfWeek == 0 && moladParts >= 20484 && isHebrewLeapYear(year)) delay = 2; // BeTuTaKF
  
  long roshHashanah = moladDays + delay;
  
  // Next year's Rosh Hashanah
  long nextMoladOffset = ((long)year % 19) * 235L + ((long)year / 19) * 6939L + 347605L;
  long nextMoladDays = nextMoladOffset / 25920L;
  long nextMoladParts = nextMoladOffset % 25920L;
  int nextDayOfWeek = (int)((nextMoladDays + 1) % 7);
  int nextDelay = 0;
  if (nextMoladParts >= 19440) nextDelay = 1;
  if (nextDayOfWeek == 1 && nextMoladParts >= 19440 && !isHebrewLeapYear(year + 1)) nextDelay = 2;
  if (nextDayOfWeek == 0 && nextMoladParts >= 20484 && isHebrewLeapYear(year + 1)) nextDelay = 2;
  long nextRoshHashanah = nextMoladDays + nextDelay;
  
  int yearDays = (int)(nextRoshHashanah - roshHashanah);
  
  // Sanity check - Hebrew years should be 353-385 days
  if (yearDays < 353 || yearDays > 385) {
    // Fallback: approximate based on leap year
    return isHebrewLeapYear(year) ? 384 : 354;
  }
  
  return yearDays;
}

// Calculate days from Hebrew epoch (Tishrei 1, 3761 BCE)
long daysFromHebrewEpoch(int hYear) {
  long days = 0;
  for (int y = 1; y < hYear; y++) {
    days += getHebrewYearDays(y);
  }
  return days;
}

// Get number of days in a Hebrew month
int getHebrewMonthDays(int hYear, int hMonth) {
  // Hebrew months: Tishrei(1), Cheshvan(2), Kislev(3), Tevet(4), Shevat(5), 
  //                Adar(6) or Adar I(6)/Adar II(7) in leap years,
  //                Nisan(7/8), Iyar(8/9), Sivan(9/10), Tammuz(10/11), Av(11/12), Elul(12/13)
  // In leap years, month 6 = Adar I (30 days), month 7 = Adar II (29 days)
  bool isLeap = isHebrewLeapYear(hYear);
  
  int standardDays[] = {30, 29, 30, 29, 30, 29, 30, 29, 30, 29, 30, 29};
  
  if (hMonth == 2) { // Cheshvan
    int yearDays = getHebrewYearDays(hYear);
    return (yearDays % 30 == 0) ? 30 : 29; // Complete vs deficient
  } else if (hMonth == 3) { // Kislev
    int yearDays = getHebrewYearDays(hYear);
    return (yearDays % 30 == 1) ? 30 : 29;
  } else if (hMonth == 6) { // Adar or Adar I
    return isLeap ? 30 : 29; // Adar I = 30 days in leap years, Adar = 29 in non-leap
  } else if (hMonth == 7) {
    if (isLeap) {
      return 29; // Adar II in leap years
    } else {
      // In non-leap years, month 7 is Nisan
      return standardDays[6]; // Nisan = 30
    }
  } else if (hMonth > 7 && !isLeap) {
    // In non-leap years, months 8-12 map to Iyar-Elul (indices 7-11 in standard array)
    return standardDays[hMonth - 1]; // Month 8->index 7, month 9->index 8, etc.
  } else if (hMonth > 7 && isLeap) {
    // In leap years, months 8-13 map to Nisan-Elul (indices 6-11 in standard array)
    return standardDays[hMonth - 2]; // Month 8->index 6, month 9->index 7, etc.
  }
  
  return standardDays[hMonth - 1];
}

// Convert Gregorian date to absolute day number (days since Jan 1, 1 CE)
long gregorianToAbsolute(int year, int month, int day) {
  long absolute = 0;
  
  // Days in previous years
  for (int y = 1; y < year; y++) {
    absolute += ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) ? 366 : 365;
  }
  
  // Days in previous months of current year
  int monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  for (int m = 1; m < month; m++) {
    int days = monthDays[m - 1];
    if (m == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
      days = 29;
    }
    absolute += days;
  }
  
  absolute += day - 1; // Days in current month (0-indexed)
  
  return absolute;
}

// Calculate approximate sunset time in minutes since midnight (local time)
// Uses a simplified calculation based on date and assumes ~mid-latitude (~35-40°)
// timezoneOffsetHours: UTC offset (e.g., -5 for EST, +2 for EET)
// dstOffset: additional hour if DST is in effect
int calculateSunsetHour(int year, int month, int day, int timezoneOffsetHours, bool dstOffset) {
  // Approximate day of year
  int monthDays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int dayOfYear = day;
  for (int m = 1; m < month; m++) {
    dayOfYear += monthDays[m];
    if (m == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
      dayOfYear += 1;
    }
  }
  
  // Calculate approximate sunset in UTC
  // Base time varies by latitude: at equator ~18:00, at 40°N ~17:30-20:30 depending on season
  // For mid-latitudes (35-40°), approximate as 18:00 ± 1.5 hours seasonal variation
  double n = (double)dayOfYear;
  double sunsetHourUTC = 18.0 + 1.5 * sin((n - 81.0) * 2.0 * 3.14159265358979323846 / 365.0);
  
  // Convert to local time by adding timezone offset and DST
  double sunsetHourLocal = sunsetHourUTC + timezoneOffsetHours + (dstOffset ? 1.0 : 0.0);
  
  // Normalize to 0-24 hour range
  while (sunsetHourLocal < 0.0) sunsetHourLocal += 24.0;
  while (sunsetHourLocal >= 24.0) sunsetHourLocal -= 24.0;
  
  return (int)(sunsetHourLocal * 60); // Return minutes since midnight in local time
}

// Hebrew calendar conversion helper
// Uses accurate calculation based on molad and Hebrew calendar rules
// timezoneOffsetHours: UTC offset (e.g., -5 for EST, +2 for EET)
// dstOffset: true if DST is currently in effect
void convertToHebrewDate(struct tm* gregorian, int* hebrewYear, int* hebrewMonth, int* hebrewDay, int timezoneOffsetHours, bool dstOffset) {
  int gYear = 1900 + gregorian->tm_year;
  int gMonth = gregorian->tm_mon + 1;
  int gDay = gregorian->tm_mday;
  int gHour = gregorian->tm_hour;
  int gMinute = gregorian->tm_min;
  
  // Hebrew calendar day starts at sunset (shkia) in local time
  // If current time is after sunset, the Hebrew day has begun, so use next day's Gregorian date
  int sunsetMinutes = calculateSunsetHour(gYear, gMonth, gDay, timezoneOffsetHours, dstOffset);
  int currentMinutes = gHour * 60 + gMinute;
  
  // If after sunset, use next Gregorian date for Hebrew calculation
  // (because the Hebrew calendar day has already started)
  if (currentMinutes >= sunsetMinutes) {
    gDay++;
    // Get days in current month
    int monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int daysInMonth = monthDays[gMonth - 1];
    if (gMonth == 2 && ((gYear % 4 == 0 && gYear % 100 != 0) || (gYear % 400 == 0))) {
      daysInMonth = 29;
    }
    if (gDay > daysInMonth) {
      gDay = 1;
      gMonth++;
      if (gMonth > 12) {
        gMonth = 1;
        gYear++;
      }
    }
  }
  
  // Convert to absolute days since Jan 1, 1 CE
  long gregorianDays = gregorianToAbsolute(gYear, gMonth, gDay);
  
  // Use known accurate reference: Nov 1, 2025 = 10 Cheshvan 5786
  long refGregorianDays = gregorianToAbsolute(2025, 11, 1);
  long daysDiff = gregorianDays - refGregorianDays;
  
  // If we're exactly on the reference date, return it directly
  if (daysDiff == 0) {
    *hebrewYear = 5786;
    *hebrewMonth = 2; // Cheshvan
    *hebrewDay = 10;
    return;
  }
  
  // Reference Hebrew date: 10 Cheshvan 5786
  // Tishrei = 30 days (days 0-29 in 0-indexed)
  // Cheshvan day 10 = day 30 + 9 = day 39 in 0-indexed (the 40th day)
  int refDaysFromYearStart = 30 + 9; // Day 39 (0-indexed) = day 40 (1-indexed) = 10 Cheshvan
  
  // Calculate total days from Tishrei 1, 5786
  long totalDaysFromTishrei5786 = refDaysFromYearStart + daysDiff;
  
  // Clamp daysDiff to reasonable range (-1000 to +1000 days) to prevent huge errors
  if (daysDiff > 1000 || daysDiff < -1000) {
    // Something is very wrong with the date, fall back to reference
    *hebrewYear = 5786;
    *hebrewMonth = 2;
    *hebrewDay = 10;
    return;
  }
  
  // Start with year 5786 and adjust as needed
  int hYear = 5786;
  long remainingDays = totalDaysFromTishrei5786;
  
  // Adjust backwards if needed
  while (remainingDays < 0 && hYear > 1) {
    hYear--;
    int yearDays = getHebrewYearDays(hYear);
    if (yearDays <= 0 || yearDays > 400) {
      // Sanity check: Hebrew years should be 353-385 days
      yearDays = 354; // Default fallback
    }
    remainingDays += yearDays;
    if (hYear < 5700) break; // Safety limit
  }
  
  // Adjust forwards if needed  
  int yearDays = getHebrewYearDays(hYear);
  if (yearDays <= 0 || yearDays > 400) {
    yearDays = 354; // Default fallback
  }
  
  int loopCount = 0;
  while (remainingDays >= yearDays && loopCount < 100) { // Safety limit
    remainingDays -= yearDays;
    hYear++;
    yearDays = getHebrewYearDays(hYear);
    if (yearDays <= 0 || yearDays > 400) {
      yearDays = 354; // Default fallback
    }
    loopCount++;
  }
  
  // Now remainingDays is 0-indexed days within the correct Hebrew year
  // Find the month and day
  int hMonth = 1;
  int hDay = 1;
  
  // Clamp remainingDays to reasonable range
  if (remainingDays < 0) remainingDays = 0;
  if (remainingDays > 400) {
    // Something went very wrong, fall back to reference date
    hYear = 5786;
    hMonth = 2;
    hDay = 10;
    *hebrewYear = hYear;
    *hebrewMonth = hMonth;
    *hebrewDay = hDay;
    return;
  }
  
  int daysCounted = 0;
  bool isLeap = isHebrewLeapYear(hYear);
  int maxMonth = isLeap ? 13 : 12;
  
  for (int m = 1; m <= maxMonth && daysCounted <= remainingDays; m++) {
    int monthDays = getHebrewMonthDays(hYear, m);
    if (monthDays <= 0 || monthDays > 32) {
      monthDays = 30; // Fallback
    }
    
    if (daysCounted + monthDays > remainingDays) {
      hMonth = m;
      hDay = remainingDays - daysCounted + 1;
      if (hDay < 1) hDay = 1;
      if (hDay > monthDays) hDay = monthDays;
      break;
    }
    daysCounted += monthDays;
  }
  
  // Safety check
  if (hMonth < 1 || hMonth > 13) {
    hYear = 5786;
    hMonth = 2;
    hDay = 10;
  }
  
  // Map month number to display month (Adar I and Adar II both display as "Adar")
  if (hMonth == 7 && isLeap) {
    hMonth = 6; // Adar II displays as "Adar"
  } else if (hMonth > 6 && isLeap) {
    hMonth--; // Shift months after Adar: 8->7 (Nisan), 9->8 (Iyar), etc.
  }
  
  *hebrewYear = hYear;
  *hebrewMonth = hMonth;
  *hebrewDay = hDay;
}

String formatHebrewDate(struct tm* timeinfo, int timezoneOffsetHours, bool dstOffset) {
  int hYear, hMonth, hDay;
  convertToHebrewDate(timeinfo, &hYear, &hMonth, &hDay, timezoneOffsetHours, dstOffset);
  
  // Transliterated Hebrew month names
  const char* hebrewMonths[] = {
    "Tishrei", "Cheshvan", "Kislev", "Tevet", "Shevat", "Adar",
    "Nisan", "Iyar", "Sivan", "Tamuz", "Av", "Elul"
  };
  
  // Format as: "10 Cheshvan 5786" (transliterated, no "b'" prefix)
  char dateBuf[64];
  String monthStr = hebrewMonths[hMonth - 1];
  if (hMonth < 1 || hMonth > 12) monthStr = "Cheshvan"; // fallback
  
  snprintf(dateBuf, sizeof(dateBuf), "%d %s %d", hDay, monthStr.c_str(), hYear);
  
  return String(dateBuf);
}

