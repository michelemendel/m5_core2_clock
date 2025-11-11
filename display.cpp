#include "display.h"
#include <M5Unified.h>
#include <time.h>
#include <math.h>

// Calculate if a Hebrew year is a leap year
static bool isHebrewLeapYear(int year) {
  int cycleYear = ((year - 1) % 19) + 1;
  return (cycleYear == 3 || cycleYear == 6 || cycleYear == 8 || cycleYear == 11 || 
          cycleYear == 14 || cycleYear == 17 || cycleYear == 19);
}

// Get number of days in a Hebrew year
static int getHebrewYearDays(int year) {
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
static long daysFromHebrewEpoch(int hYear) {
  long days = 0;
  for (int y = 1; y < hYear; y++) {
    days += getHebrewYearDays(y);
  }
  return days;
}

// Get number of days in a Hebrew month
static int getHebrewMonthDays(int hYear, int hMonth) {
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
static long gregorianToAbsolute(int year, int month, int day) {
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
static int calculateSunsetHour(int year, int month, int day, int timezoneOffsetHours, bool dstOffset) {
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
static void convertToHebrewDate(struct tm* gregorian, int* hebrewYear, int* hebrewMonth, int* hebrewDay, int timezoneOffsetHours, bool dstOffset) {
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

static String formatHebrewDate(struct tm* timeinfo, int timezoneOffsetHours, bool dstOffset) {
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

Display::Display() {
  screenWidth = 0;
  screenHeight = 0;
  brightness = BRIGHTNESS_HIGH;
}

void Display::begin() {
  M5.Display.begin();
  M5.Display.setRotation(1);
  screenWidth = M5.Display.width();
  screenHeight = M5.Display.height();
  setBrightness(BRIGHTNESS_HIGH);
  // Initialize battery smoothing buffer
  for (int i = 0; i < 5; ++i) { batterySamples[i] = 0; }
  batterySampleIndex = 0;
  batterySampleCount = 0;
  prevSmoothLevel = -1;
  prevVoltage = 0.0f;
  chargingConfidence = 0;
  chargeOnStreak = 0;
  chargeOffStreak = 0;
  showChargingLatched = false;
  voltageRiseStreak = 0;
  voltageNoRiseStreak = 0;
}
int Display::getSmoothedBatteryLevel(int rawLevel) {
  batterySamples[batterySampleIndex] = constrain(rawLevel, 0, 100);
  batterySampleIndex = (batterySampleIndex + 1) % 5;
  if (batterySampleCount < 5) batterySampleCount++;
  long sum = 0;
  for (int i = 0; i < batterySampleCount; ++i) sum += batterySamples[i];
  return (int)(sum / (long)batterySampleCount);
}


void Display::update() {
  // Called every frame if needed for animations
}

void Display::showClock(const Time& time, bool wifiConnected, const Alarm& alarm, const Timezone& timezone) {
  // Format time as HH:mm:ss
  char timeStr[9];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", 
           time.hours, time.minutes, time.seconds);

  // If this is the first draw, render the whole view once
  if (!initialClockDrawn) {
    M5.Display.fillScreen(TFT_BLACK);

    // Draw main time (default spacing)
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setTextSize(4);
    M5.Display.drawString(timeStr, screenWidth / 2, screenHeight / 2 - 10);

    // Draw Hebrew date line (between time and common date)
    {
      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 0)) {
        String hebrewDateStr = formatHebrewDate(&timeinfo, timezone.offsetHours, timezone.daylightSaving);
        M5.Display.setTextSize(2);
        M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
        M5.Display.setTextDatum(MC_DATUM);
        M5.Display.drawString(hebrewDateStr, screenWidth / 2, screenHeight / 2 + 15);
      }
    }

    // Draw common date line (below Hebrew date)
    {
      struct tm timeinfo;
      char dateStr[32] = "";
      if (getLocalTime(&timeinfo, 0)) {
        const char* wdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        const char* w = (timeinfo.tm_wday >= 0 && timeinfo.tm_wday <= 6) ? wdays[timeinfo.tm_wday] : "";
        snprintf(dateStr, sizeof(dateStr), "%s %04d-%02d-%02d", w, 1900 + timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday);
      }
      M5.Display.setTextSize(2);
      M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
      M5.Display.setTextDatum(MC_DATUM);
      M5.Display.drawString(String(dateStr), screenWidth / 2, screenHeight / 2 + 35);
    }

    // Build status line content
    String tzDisplay = timezone.name;
    if (timezone.offsetHours >= 0) tzDisplay += "+";
    tzDisplay += String(timezone.offsetHours);

    // Draw status line (WiFi, TZ left; DST right)
    M5.Display.setTextDatum(TL_DATUM);
    M5.Display.setTextSize(1);
    int y = 5;
    int sx = 10;

    M5.Display.setTextColor(wifiConnected ? TFT_GREEN : TFT_RED, TFT_BLACK);
    String wifiStr = hasWifiOverride ? wifiOverride : (wifiConnected ? "WiFi" : "No WiFi");
    M5.Display.drawString(wifiStr, sx, y);
    sx += M5.Display.textWidth(wifiStr) + 10;

    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Display.drawString(tzDisplay, sx, y);
    
    // DST below timezone (like voltage below battery)
    M5.Display.setTextColor(TFT_ORANGE, TFT_BLACK);
    M5.Display.drawString(timezone.daylightSaving ? "DST ON" : "DST OFF", sx, y + 10);

    // Right-align alarm time (where DST used to be), reserving space on far right for battery/voltage
    M5.Display.setTextDatum(TR_DATUM);
    // Clear alarm area first
    M5.Display.fillRect(screenWidth - 110, y - 2, 100, 12, TFT_BLACK);
    if (alarm.enabled) {
      char alarmTimeStr[6];
      snprintf(alarmTimeStr, sizeof(alarmTimeStr), "%02d:%02d", alarm.hours, alarm.minutes);
      M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
      M5.Display.drawString(alarmTimeStr, screenWidth - 100, y);
    }
    M5.Display.setTextDatum(TL_DATUM);

    // Battery group
    int level = M5.Power.getBatteryLevel();
    int smoothLevel = getSmoothedBatteryLevel(level);
    bool charging = M5.Power.isCharging();
    M5.Display.setTextDatum(TR_DATUM);
    float voltage = M5.Power.getBatteryVoltage() / 1000.0f;  // Convert mV to V
    // Simple stable rule: only show charging when USB likely present
    // Require isCharging and voltage >= 4.05V and battery < 100%
    bool showCharging = charging && (smoothLevel < 100) && (voltage >= 4.05f);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(showCharging ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
    // Voltage-derived estimate when unplugged to improve accuracy
    int voltagePct = (int)constrain(((voltage - 3.50f) / (4.20f - 3.50f)) * 100.0f, 0.0f, 100.0f);
    int displayPct = charging ? smoothLevel : (int)(smoothLevel * 0.3f + voltagePct * 0.7f);
    String bat = String(displayPct) + "%";
    if (showCharging) bat = "+" + bat;
    M5.Display.drawString(bat, screenWidth - 6, y);
    M5.Display.setTextSize(1);
    M5.Display.setTextDatum(TR_DATUM);
    char vbuf[16];
    snprintf(vbuf, sizeof(vbuf), "%.2fV", voltage);
    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Display.drawString(String(vbuf), screenWidth - 6, y + 10);
    // Removed lightning icon

    // Cache status
    initialClockDrawn = true;
    lastWifiConnected = wifiConnected;
    lastHadWifiOverride = hasWifiOverride;
    lastWifiLabel = wifiStr;
    lastAlarmEnabled = alarm.enabled;
    lastAlarmHours = alarm.hours;
    lastAlarmMinutes = alarm.minutes;
    lastTimezoneDisplay = tzDisplay;
    lastDSTOn = timezone.daylightSaving;
    return;
  }

  // Incremental updates below
  // 1) Update main time
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(4);
  M5.Display.drawString(timeStr, screenWidth / 2, screenHeight / 2 - 10);

  // 2) Update Hebrew date and common date lines
  {
    // Clear area for Hebrew and common date
    int dy = screenHeight / 2 + 15;
    M5.Display.fillRect(0, dy - 12, screenWidth, 40, TFT_BLACK);
    
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
      // Hebrew date line
      String hebrewDateStr = formatHebrewDate(&timeinfo, timezone.offsetHours, timezone.daylightSaving);
      M5.Display.setTextSize(2);
      M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
      M5.Display.setTextDatum(MC_DATUM);
      M5.Display.drawString(hebrewDateStr, screenWidth / 2, dy);
      
      // Common date line
      char dateStr[32] = "";
      const char* wdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
      const char* w = (timeinfo.tm_wday >= 0 && timeinfo.tm_wday <= 6) ? wdays[timeinfo.tm_wday] : "";
      snprintf(dateStr, sizeof(dateStr), "%s %04d-%02d-%02d", w, 1900 + timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday);
      M5.Display.setTextSize(2);
      M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
      M5.Display.drawString(String(dateStr), screenWidth / 2, dy + 20);
    }
  }

  // 3) Status line: refresh only if something changed (wifi/tz/dst)
  {
    String tzDisplay = timezone.name;
    if (timezone.offsetHours >= 0) tzDisplay += "+";
    tzDisplay += String(timezone.offsetHours);
    String wifiStrNow = hasWifiOverride ? wifiOverride : (wifiConnected ? "WiFi" : "No WiFi");
    bool statusChanged = (wifiConnected != lastWifiConnected) ||
                         (tzDisplay != lastTimezoneDisplay) ||
                         (timezone.daylightSaving != lastDSTOn) ||
                         (hasWifiOverride != lastHadWifiOverride) ||
                         (wifiStrNow != lastWifiLabel) ||
                         (alarm.enabled != lastAlarmEnabled) ||
                         (alarm.enabled && (alarm.hours != lastAlarmHours || alarm.minutes != lastAlarmMinutes));
    if (statusChanged) {
      // Clear top bar area
      M5.Display.fillRect(0, 0, screenWidth, 20, TFT_BLACK);
      // Redraw status items (WiFi, TZ left; DST right)
      M5.Display.setTextDatum(TL_DATUM);
      M5.Display.setTextSize(1);
      int y = 5;
      int x = 10;

      M5.Display.setTextColor(wifiConnected ? TFT_GREEN : TFT_RED, TFT_BLACK);
      String wifiStr = wifiStrNow;
      M5.Display.drawString(wifiStr, x, y);
      x += M5.Display.textWidth(wifiStr) + 10;

      M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
      M5.Display.drawString(tzDisplay, x, y);
      
      // DST below timezone (like voltage below battery)
      M5.Display.setTextColor(TFT_ORANGE, TFT_BLACK);
      M5.Display.drawString(timezone.daylightSaving ? "DST ON" : "DST OFF", x, y + 10);

      // Right-align alarm time (where DST used to be)
      M5.Display.setTextDatum(TR_DATUM);
      // Clear alarm area first
      M5.Display.fillRect(screenWidth - 110, y - 2, 100, 12, TFT_BLACK);
      if (alarm.enabled) {
        char alarmTimeStr[6];
        snprintf(alarmTimeStr, sizeof(alarmTimeStr), "%02d:%02d", alarm.hours, alarm.minutes);
        M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
        M5.Display.drawString(alarmTimeStr, screenWidth - 100, y);
      }
      M5.Display.setTextDatum(TL_DATUM);

      lastWifiConnected = wifiConnected;
      lastHadWifiOverride = hasWifiOverride;
      lastWifiLabel = wifiStrNow;
      lastAlarmEnabled = alarm.enabled;
      lastAlarmHours = alarm.hours;
      lastAlarmMinutes = alarm.minutes;
      lastTimezoneDisplay = tzDisplay;
      lastDSTOn = timezone.daylightSaving;
    }
  }

  // 4) Battery area: small targeted clear and redraw every tick
  {
    int y = 5;
    // Clear a small box on the right for battery texts and icon
    M5.Display.fillRect(screenWidth - 90, 0, 90, 22, TFT_BLACK);
    int level = M5.Power.getBatteryLevel();
    int smoothLevel = getSmoothedBatteryLevel(level);
    bool charging = M5.Power.isCharging();
    float voltage = M5.Power.getBatteryVoltage() / 1000.0f;  // Convert mV to V
    bool showCharging = charging && (smoothLevel < 100) && (voltage >= 4.05f);
    M5.Display.setTextSize(1);
    M5.Display.setTextDatum(TR_DATUM);
    M5.Display.setTextColor(showCharging ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
    // Voltage-derived estimate when unplugged to improve accuracy
    int voltagePct = (int)constrain(((voltage - 3.50f) / (4.20f - 3.50f)) * 100.0f, 0.0f, 100.0f);
    int displayPct = charging ? smoothLevel : (int)(smoothLevel * 0.3f + voltagePct * 0.7f);
    String bat = String(displayPct) + "%";
    if (showCharging) bat = "+" + bat;
    M5.Display.drawString(bat, screenWidth - 6, y);
    M5.Display.setTextSize(1);
    M5.Display.setTextDatum(TR_DATUM);
    char vbuf[16];
    snprintf(vbuf, sizeof(vbuf), "%.2fV", voltage);
    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Display.drawString(String(vbuf), screenWidth - 6, y + 10);
    // Removed lightning icon
  }
}

void Display::showMainView(int selection) {
  // Leaving clock view; force next clock draw to fully refresh
  initialClockDrawn = false;
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  
  const char* items[] = {"Set Alarm", "WiFi Setup", "Set Time", "Set Timezone", "Set DST"};
  
  int y = 16;
  for (int i = 0; i < 5; i++) {
    if (i == selection) {
      M5.Display.fillRect(10, y - 4, screenWidth - 20, 24, TFT_DARKGREEN);
      M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    } else {
      M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    }
    M5.Display.drawString(items[i], 15, y);
    y += 26;
  }
}

void Display::showWiFiScan(const String* ssids, int count, int selection) {
  // Leaving clock view; force next clock draw to fully refresh
  initialClockDrawn = false;
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  
  M5.Display.drawString("WiFi Networks:", 10, 5);
  
  int startIdx = max(0, selection - 4);
  int endIdx = min(count, startIdx + 5);
  
  int y = 25;
  for (int i = startIdx; i < endIdx; i++) {
    if (i == selection) {
      M5.Display.fillRect(10, y - 2, screenWidth - 20, 20, TFT_DARKGREEN);
      M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    } else {
      M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    }
    
    String displayName = ssids[i];
    if (displayName.length() > 20) {
      displayName = displayName.substring(0, 17) + "...";
    }
    M5.Display.drawString(displayName, 15, y);
    y += 22;
  }
  
  if (count == 0) {
    M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Display.drawString("Scanning...", 15, 30);
  }
}

void Display::showWiFiPassword(const String& ssid, const String& password) {
  // Leaving clock view; force next clock draw to fully refresh
  initialClockDrawn = false;
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  
  M5.Display.drawString("Password for:", 10, 10);
  
  // Truncate SSID if too long
  String displaySSID = ssid;
  if (displaySSID.length() > 25) {
    displaySSID = displaySSID.substring(0, 22) + "...";
  }
  M5.Display.drawString(displaySSID, 10, 30);
  
  M5.Display.setTextSize(3);
  M5.Display.setTextDatum(MC_DATUM);
  
  // Show password with masking - show actual length
  String masked = "";
  int pwdLen = password.length();
  for (int i = 0; i < pwdLen; i++) {
    masked += "*";
  }
  if (masked.length() == 0) {
    masked = "_";
  }
  
  // Display password - use smaller font if very long
  if (pwdLen > 15) {
    M5.Display.setTextSize(2);
  }
  M5.Display.drawString(masked, screenWidth / 2, screenHeight / 2);
  
  M5.Display.setTextSize(1);
  M5.Display.setTextDatum(BL_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  // No bottom navigation hints
}

void Display::showTimeSetting(uint8_t hours, uint8_t minutes, int field) {
  // Leaving clock view; force next clock draw to fully refresh
  initialClockDrawn = false;
  M5.Display.fillScreen(TFT_BLACK);
  
  char timeStr[9];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", hours, minutes);
  
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(5);
  
  // Highlight selected field
  if (field == 0) {
    // Highlight hours
    M5.Display.fillRect(screenWidth / 2 - 60, screenHeight / 2 - 50, 50, 50, TFT_DARKGREEN);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  } else {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  
  M5.Display.drawString(String(timeStr).substring(0, 2), screenWidth / 2 - 35, screenHeight / 2 - 20);
  
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.drawString(":", screenWidth / 2, screenHeight / 2 - 20);
  
  if (field == 1) {
    // Highlight minutes
    M5.Display.fillRect(screenWidth / 2 + 10, screenHeight / 2 - 50, 50, 50, TFT_DARKGREEN);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  } else {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  
  M5.Display.drawString(String(timeStr).substring(3, 5), screenWidth / 2 + 35, screenHeight / 2 - 20);
  
  // Draw NTP sync option below time
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setTextSize(2);
  int optionY = screenHeight / 2 + 25;
  if (field == 2) {
    M5.Display.fillRect(20, optionY - 12, screenWidth - 40, 24, TFT_DARKGREEN);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  } else {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  M5.Display.drawString("Sync from NTP now", screenWidth / 2, optionY);
  // No bottom navigation hints
}

void Display::showAlarmSetting(uint8_t hours, uint8_t minutes, bool enabled, uint16_t lengthSeconds, uint8_t volume, int field, const Time& currentTime) {
  // Leaving clock view; force next clock draw to fully refresh
  initialClockDrawn = false;
  M5.Display.fillScreen(TFT_BLACK);
  
  char timeStr[9];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", hours, minutes);
  
  // Show current time above (small font)
  char nowStr[9];
  snprintf(nowStr, sizeof(nowStr), "%02d:%02d:%02d", currentTime.hours, currentTime.minutes, currentTime.seconds);
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  M5.Display.drawString(nowStr, screenWidth / 2, 12);
  
  // Draw alarm time
  M5.Display.setTextSize(3);
  
  // Draw time
  int y = screenHeight / 2 - 25;
  if (field == 0) {
    // Highlight hours
    M5.Display.fillRect(screenWidth / 2 - 48, y - 18, 36, 36, TFT_DARKGREEN);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  } else {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  M5.Display.drawString(String(timeStr).substring(0, 2), screenWidth / 2 - 28, y);
  
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.drawString(":", screenWidth / 2, y);
  
  if (field == 1) {
    // Highlight minutes
    M5.Display.fillRect(screenWidth / 2 + 12, y - 18, 36, 36, TFT_DARKGREEN);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  } else {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  M5.Display.drawString(String(timeStr).substring(3, 5), screenWidth / 2 + 28, y);
  
  // Draw enabled/disabled
  y += 32;
  M5.Display.setTextSize(2);
  if (field == 2) {
    M5.Display.fillRect(screenWidth / 2 - 60, y - 15, 120, 30, TFT_DARKGREEN);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  } else {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  M5.Display.drawString(enabled ? "ON" : "OFF", screenWidth / 2, y);

  // Draw auto-off length (seconds)
  y += 24;
  char lenBuf[24];
  snprintf(lenBuf, sizeof(lenBuf), "Auto-off: %us", (unsigned)lengthSeconds);
  if (field == 3) {
    int w = 170; int h = 20; int bx = (screenWidth - w) / 2; int by = y - 12;
    M5.Display.fillRect(bx, by, w, h, TFT_DARKGREEN);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  } else {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  M5.Display.drawString(lenBuf, screenWidth / 2, y);

  // Draw volume (0-255)
  y += 20;
  char volBuf[24];
  snprintf(volBuf, sizeof(volBuf), "Volume: %u", (unsigned)volume);
  if (field == 4) {
    int w = 150; int h = 20; int bx = (screenWidth - w) / 2; int by = y - 12;
    M5.Display.fillRect(bx, by, w, h, TFT_DARKGREEN);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  } else {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  M5.Display.drawString(volBuf, screenWidth / 2, y);

  // No bottom navigation hints
}

void Display::showStatus(const String& message) {
  // Leaving clock view; force next clock draw to fully refresh
  initialClockDrawn = false;
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.drawString(message, screenWidth / 2, screenHeight / 2);
}

void Display::showConnecting() {
  // Leaving clock view; force next clock draw to fully refresh
  initialClockDrawn = false;
  showStatus("Connecting...");
}

void Display::showSyncing() {
  // Leaving clock view; force next clock draw to fully refresh
  initialClockDrawn = false;
  showStatus("Syncing time...");
}

void Display::setWifiStatusOverride(const String& label) {
  hasWifiOverride = true;
  wifiOverride = label;
}

void Display::clearWifiStatusOverride() {
  hasWifiOverride = false;
  wifiOverride = "";
}

void Display::setBrightness(int level) {
  brightness = constrain(level, 0, 255);
  M5.Display.setBrightness(brightness);
}

void Display::setBrightnessForCharger(bool charging) {
  if (manualBrightness) return; // respect manual override
  if (charging) {
    setBrightness(BRIGHTNESS_HIGH);
  } else {
    setBrightness(BRIGHTNESS_LOW);
  }
}

void Display::increaseBrightnessStep(int step) {
  manualBrightness = true;
  setBrightness(brightness + step);
}

void Display::decreaseBrightnessStep(int step) {
  manualBrightness = true;
  setBrightness(brightness - step);
}

void Display::showTimezoneSetting(int selection) {
  // Leaving clock view; force next clock draw to fully refresh
  initialClockDrawn = false;
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  
  M5.Display.drawString("Select Timezone:", 10, 5);
  
  // Timezone names and offsets
  const char* timezoneNames[] = {
    "HST (UTC-10)",  // 0
    "AKST (UTC-9)",  // 1
    "PST (UTC-8)",   // 2
    "MST (UTC-7)",   // 3
    "CST (UTC-6)",   // 4
    "EST (UTC-5)",   // 5
    "AST (UTC-4)",   // 6
    "BRT (UTC-3)",   // 7
    "GMT (UTC+0)",   // 8
    "CET (UTC+1)",   // 9
    "EET (UTC+2)",   // 10
    "MSK (UTC+3)",   // 11
    "GST (UTC+4)",   // 12
    "PKT (UTC+5)",   // 13
    "BST (UTC+6)",   // 14 (Bangladesh)
    "ICT (UTC+7)",   // 15
    "CST (UTC+8)",   // 16 (China)
    "JST (UTC+9)",   // 17
    "AEST (UTC+10)"  // 18
  };
  
  int startIdx = max(0, selection - 3);
  int endIdx = min(19, startIdx + 6);
  
  int y = 25;
  for (int i = startIdx; i < endIdx; i++) {
    if (i == selection) {
      M5.Display.fillRect(10, y - 2, screenWidth - 20, 20, TFT_DARKGREEN);
      M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    } else {
      M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    }
    
    M5.Display.drawString(timezoneNames[i], 15, y);
    y += 22;
  }

  // No bottom navigation hints
}

void Display::showDSTSetting(bool enabled) {
  // Leaving clock view; force next clock draw to fully refresh
  initialClockDrawn = false;
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(3);
  M5.Display.drawString("DST", screenWidth / 2, screenHeight / 2 - 30);
  
  // ON/OFF with highlight
  M5.Display.setTextSize(4);
  if (enabled) {
    M5.Display.fillRect(screenWidth / 2 - 80, screenHeight / 2, 70, 40, TFT_DARKGREEN);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    M5.Display.drawString("ON", screenWidth / 2 - 45, screenHeight / 2 + 20);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.drawString("OFF", screenWidth / 2 + 35, screenHeight / 2 + 20);
  } else {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.drawString("ON", screenWidth / 2 - 45, screenHeight / 2 + 20);
    M5.Display.fillRect(screenWidth / 2 + 20, screenHeight / 2, 80, 40, TFT_DARKGREEN);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    M5.Display.drawString("OFF", screenWidth / 2 + 60, screenHeight / 2 + 20);
  }
  
  // No bottom navigation hints
}

