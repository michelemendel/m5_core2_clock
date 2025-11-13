#include "display.h"
#include "view.h"
#include "calendar.h"
#include <M5Unified.h>
#include <Preferences.h>
#include <time.h>
#include <math.h>
#include <string.h>

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
  // Load saved brightness, or use default
  loadBrightness();
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

    // Draw main time with segment-style font (digital clock look)
    // Clear time area first to remove any artifacts - use larger clear area
    int timeY = screenHeight / 2 - 20;
    M5.Display.fillRect(0, timeY - 50, screenWidth, 100, TFT_BLACK);
    M5.Display.setFont(&fonts::Font7);  // Seven-segment style font
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.drawString(timeStr, screenWidth / 2, timeY);

    // Draw Hebrew date line (more spacing from time) - use smaller nice font
    {
      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 0)) {
        String hebrewDateStr = formatHebrewDate(&timeinfo, timezone.offsetHours, timezone.daylightSaving);
        M5.Display.setFont(&fonts::FreeSans9pt7b);  // Smaller smooth font for Hebrew date
        M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
        M5.Display.setTextDatum(MC_DATUM);
        M5.Display.drawString(hebrewDateStr, screenWidth / 2, screenHeight / 2 + 45);
      }
    }

    // Draw common date line (more spacing from Hebrew date) - use smaller nice font
    {
      struct tm timeinfo;
      char dateStr[32] = "";
      if (getLocalTime(&timeinfo, 0)) {
        const char* wdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        const char* w = (timeinfo.tm_wday >= 0 && timeinfo.tm_wday <= 6) ? wdays[timeinfo.tm_wday] : "";
        snprintf(dateStr, sizeof(dateStr), "%s %04d-%02d-%02d", w, 1900 + timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday);
      }
      M5.Display.setFont(&fonts::FreeSans9pt7b);  // Smaller smooth font for common date
      M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
      M5.Display.setTextDatum(MC_DATUM);
      M5.Display.drawString(String(dateStr), screenWidth / 2, screenHeight / 2 + 75);
    }

    // Build status line content
    String tzDisplay = timezone.name;
    if (timezone.offsetHours >= 0) tzDisplay += "+";
    tzDisplay += String(timezone.offsetHours);

    // Draw status line (WiFi, TZ left; DST right) - use very small font
    M5.Display.setFont(nullptr);  // Use default font
    M5.Display.setTextSize(1);  // Smallest size for status bar
    M5.Display.setTextDatum(TL_DATUM);
    int y = 3;
    int sx = 5;

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
    M5.Display.fillRect(screenWidth - 80, y - 1, 75, 10, TFT_BLACK);
    if (alarm.enabled) {
      char alarmTimeStr[6];
      snprintf(alarmTimeStr, sizeof(alarmTimeStr), "%02d:%02d", alarm.hours, alarm.minutes);
      M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
      M5.Display.drawString(alarmTimeStr, screenWidth - 70, y);
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
    M5.Display.setFont(nullptr);  // Use default font
    M5.Display.setTextSize(1);  // Smallest size for battery
    M5.Display.setTextColor(showCharging ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
    // Voltage-derived estimate when unplugged to improve accuracy
    int voltagePct = (int)constrain(((voltage - 3.50f) / (4.20f - 3.50f)) * 100.0f, 0.0f, 100.0f);
    int displayPct = charging ? smoothLevel : (int)(smoothLevel * 0.3f + voltagePct * 0.7f);
    String bat = String(displayPct) + "%";
    if (showCharging) bat = "+" + bat;
    M5.Display.drawString(bat, screenWidth - 5, y);
    M5.Display.setFont(nullptr);  // Use default font
    M5.Display.setTextSize(1);  // Smallest size for voltage
    M5.Display.setTextDatum(TR_DATUM);
    char vbuf[16];
    snprintf(vbuf, sizeof(vbuf), "%.2fV", voltage);
    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Display.drawString(String(vbuf), screenWidth - 5, y + 10);
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
  // 1) Update main time with segment-style font - clear area first
  int timeY = screenHeight / 2 - 20;
  // Clear time area to remove any artifacts or old text - use larger clear area
  // Reset font to default before clearing to ensure proper clearing
  M5.Display.setFont(nullptr);
  M5.Display.fillRect(0, timeY - 50, screenWidth, 100, TFT_BLACK);
  // Now set the segment font and draw
  M5.Display.setFont(&fonts::Font7);  // Seven-segment style font
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.drawString(timeStr, screenWidth / 2, timeY);

  // 2) Update Hebrew date and common date lines with better spacing
  {
    // Clear area for Hebrew and common date (more space to prevent overlap)
    int dy = screenHeight / 2 + 45;
    M5.Display.fillRect(0, dy - 15, screenWidth, 60, TFT_BLACK);
    
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
      // Hebrew date line
      String hebrewDateStr = formatHebrewDate(&timeinfo, timezone.offsetHours, timezone.daylightSaving);
      M5.Display.setFont(&fonts::FreeSans9pt7b);  // Smaller smooth font for Hebrew date
      M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
      M5.Display.setTextDatum(MC_DATUM);
      M5.Display.drawString(hebrewDateStr, screenWidth / 2, dy);
      
      // Common date line (more spacing from Hebrew date to prevent overlap)
      char dateStr[32] = "";
      const char* wdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
      const char* w = (timeinfo.tm_wday >= 0 && timeinfo.tm_wday <= 6) ? wdays[timeinfo.tm_wday] : "";
      snprintf(dateStr, sizeof(dateStr), "%s %04d-%02d-%02d", w, 1900 + timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday);
      M5.Display.setFont(&fonts::FreeSans9pt7b);  // Smaller smooth font for common date
      M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
      M5.Display.drawString(String(dateStr), screenWidth / 2, dy + 30);
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
      // Clear top bar area (smaller now)
      M5.Display.fillRect(0, 0, screenWidth, 18, TFT_BLACK);
      // Redraw status items (WiFi, TZ left; DST right) - use very small font
      M5.Display.setFont(nullptr);  // Use default font
      M5.Display.setTextSize(1);  // Smallest size for status bar
      M5.Display.setTextDatum(TL_DATUM);
      int y = 3;
      int x = 5;

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
      M5.Display.fillRect(screenWidth - 80, y - 1, 75, 10, TFT_BLACK);
      if (alarm.enabled) {
        char alarmTimeStr[6];
        snprintf(alarmTimeStr, sizeof(alarmTimeStr), "%02d:%02d", alarm.hours, alarm.minutes);
        M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
        M5.Display.drawString(alarmTimeStr, screenWidth - 70, y);
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
    int y = 3;
    // Clear a small box on the right for battery texts and icon
    M5.Display.fillRect(screenWidth - 70, 0, 70, 18, TFT_BLACK);
    int level = M5.Power.getBatteryLevel();
    int smoothLevel = getSmoothedBatteryLevel(level);
    bool charging = M5.Power.isCharging();
    float voltage = M5.Power.getBatteryVoltage() / 1000.0f;  // Convert mV to V
    bool showCharging = charging && (smoothLevel < 100) && (voltage >= 4.05f);
    M5.Display.setFont(nullptr);  // Use default font
    M5.Display.setTextSize(1);  // Smallest size for battery
    M5.Display.setTextDatum(TR_DATUM);
    M5.Display.setTextColor(showCharging ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
    // Voltage-derived estimate when unplugged to improve accuracy
    int voltagePct = (int)constrain(((voltage - 3.50f) / (4.20f - 3.50f)) * 100.0f, 0.0f, 100.0f);
    int displayPct = charging ? smoothLevel : (int)(smoothLevel * 0.3f + voltagePct * 0.7f);
    String bat = String(displayPct) + "%";
    if (showCharging) bat = "+" + bat;
    M5.Display.drawString(bat, screenWidth - 5, y);
    M5.Display.setFont(nullptr);  // Use default font
    M5.Display.setTextSize(1);  // Smallest size for voltage
    M5.Display.setTextDatum(TR_DATUM);
    char vbuf[16];
    snprintf(vbuf, sizeof(vbuf), "%.2fV", voltage);
    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Display.drawString(String(vbuf), screenWidth - 5, y + 10);
    // Removed lightning icon
  }
}

void Display::showMainView(int selection) {
  // Leaving clock view; force next clock draw to fully refresh
  initialClockDrawn = false;
  M5.Display.fillScreen(TFT_BLACK);
  
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  
  const char* items[] = {"Set Alarm", "WiFi Setup", "Set Time", "Set Timezone", "Set DST", "Set Brightness"};
  
  // Start menu items at top
  int y = 16;
  for (int i = 0; i < 6; i++) {
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
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  
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
  // Don't reset keyboardShift here - let it persist so shift button works
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString("Password for:", 10, 10);
  
  // Truncate SSID if too long
  String displaySSID = ssid;
  if (displaySSID.length() > 25) {
    displaySSID = displaySSID.substring(0, 22) + "...";
  }
  M5.Display.drawString(displaySSID, 10, 30);
  
  // Show password in clear text
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(TL_DATUM);
  if (password.length() == 0) {
    M5.Display.drawString("_", 10, 55);
  } else {
    M5.Display.drawString(password, 10, 55);
  }
  
  // On-screen keyboard layout (QWERTY, compact)
  // Row 1: Q W E R T Y U I O P
  // Row 2: A S D F G H J K L
  // Row 3: Z X C V B N M
  // Row 4: Shift, Space, Backspace, Enter
  const char* keyboardRowsUpper[] = {
    "QWERTYUIOP",
    "ASDFGHJKL",
    "ZXCVBNM",
    "  \b\n"
  };
  const char* keyboardRowsLower[] = {
    "qwertyuiop",
    "asdfghjkl",
    "zxcvbnm",
    "  \b\n"
  };
  const char** keyboardRows = keyboardShift ? keyboardRowsLower : keyboardRowsUpper;
  
  int keyWidth = 28;
  int keyHeight = 30;
  int keySpacing = 2;
  int startY = 85;  // Moved up since no back button
  int startX = 10;
  
  for (int row = 0; row < 4; row++) {
    int rowLen = strlen(keyboardRows[row]);
    int rowStartX = startX;
    if (row == 1) rowStartX += keyWidth / 2; // Offset second row
    if (row == 2) rowStartX += keyWidth; // Offset third row
    if (row == 3) {
      // Special keys row
      // Shift button
      M5.Display.fillRect(rowStartX, startY + row * (keyHeight + keySpacing), keyWidth * 2, keyHeight, keyboardShift ? TFT_DARKGREEN : TFT_DARKGRAY);
      M5.Display.setTextColor(TFT_WHITE, keyboardShift ? TFT_DARKGREEN : TFT_DARKGRAY);
      M5.Display.setFont(&fonts::FreeSans9pt7b);
      M5.Display.drawString("Shift", rowStartX + keyWidth, startY + row * (keyHeight + keySpacing) + keyHeight / 2);
      
      // Space (wider)
      M5.Display.fillRect(rowStartX + keyWidth * 2 + keySpacing, startY + row * (keyHeight + keySpacing), keyWidth * 3, keyHeight, TFT_DARKGRAY);
      M5.Display.setTextColor(TFT_WHITE, TFT_DARKGRAY);
      M5.Display.setFont(&fonts::FreeSans9pt7b);
      M5.Display.drawString("Space", rowStartX + keyWidth * 3.5, startY + row * (keyHeight + keySpacing) + keyHeight / 2);
      
      // Backspace
      M5.Display.fillRect(rowStartX + keyWidth * 5 + keySpacing * 2, startY + row * (keyHeight + keySpacing), keyWidth * 2, keyHeight, TFT_DARKGRAY);
      M5.Display.setFont(&fonts::FreeSans9pt7b);
      M5.Display.drawString("Del", rowStartX + keyWidth * 6, startY + row * (keyHeight + keySpacing) + keyHeight / 2);
      
      // Enter
      M5.Display.fillRect(rowStartX + keyWidth * 7 + keySpacing * 3, startY + row * (keyHeight + keySpacing), keyWidth * 2, keyHeight, TFT_DARKGREEN);
      M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
      M5.Display.setFont(&fonts::FreeSans9pt7b);
      M5.Display.drawString("Enter", rowStartX + keyWidth * 8, startY + row * (keyHeight + keySpacing) + keyHeight / 2);
      break;
    }
    
    for (int col = 0; col < rowLen; col++) {
      int x = rowStartX + col * (keyWidth + keySpacing);
      int y = startY + row * (keyHeight + keySpacing);
      M5.Display.fillRect(x, y, keyWidth, keyHeight, TFT_DARKGRAY);
      M5.Display.setTextColor(TFT_WHITE, TFT_DARKGRAY);
      M5.Display.setTextSize(2);
      M5.Display.setTextDatum(MC_DATUM);
      char keyStr[2] = {keyboardRows[row][col], '\0'};
      M5.Display.drawString(keyStr, x + keyWidth / 2, y + keyHeight / 2);
    }
  }
  
  M5.Display.setTextDatum(TL_DATUM);
}

void Display::showTimeSetting(uint8_t hours, uint8_t minutes, int field) {
  // Leaving clock view; force next clock draw to fully refresh
  initialClockDrawn = false;
  M5.Display.fillScreen(TFT_BLACK);
  
  char timeStr[9];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", hours, minutes);
  
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setFont(&fonts::Font7);  // Seven-segment style font
  
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
  M5.Display.setFont(&fonts::FreeSans9pt7b);
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
  M5.Display.setFont(&fonts::FreeSans9pt7b);  // Smaller font for current time
  M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  M5.Display.drawString(nowStr, screenWidth / 2, 25);
  
  // Draw alarm time
  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setFont(&fonts::Font7);  // Seven-segment style font
  
  // Draw time
  int y = screenHeight / 2 - 25;
  if (field == 0) {
    // Highlight hours
    M5.Display.fillRect(screenWidth / 2 - 60, y - 18, 50, 36, TFT_DARKGREEN);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  } else {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  M5.Display.drawString(String(timeStr).substring(0, 2), screenWidth / 2 - 35, y);
  
  // Draw colon as two dots (Font7 may not have colon character)
  M5.Display.fillCircle(screenWidth / 2 - 2, y - 8, 3, TFT_WHITE);
  M5.Display.fillCircle(screenWidth / 2 - 2, y + 8, 3, TFT_WHITE);
  
  if (field == 1) {
    // Highlight minutes
    M5.Display.fillRect(screenWidth / 2 + 10, y - 18, 50, 36, TFT_DARKGREEN);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  } else {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  M5.Display.drawString(String(timeStr).substring(3, 5), screenWidth / 2 + 35, y);
  
  // Draw enabled/disabled
  y += 50;
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  if (field == 2) {
    M5.Display.fillRect(screenWidth / 2 - 60, y - 15, 120, 30, TFT_DARKGREEN);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  } else {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  M5.Display.drawString(enabled ? "ON" : "OFF", screenWidth / 2, y);

  // Draw auto-off length (seconds)
  y += 30;
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
  y += 26;
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
  M5.Display.setFont(&fonts::FreeSans9pt7b);
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
  // Apply brightness immediately to the display
  M5.Display.setBrightness(brightness);
}

void Display::setBrightnessForCharger(bool charging) {
  // Always respect manual brightness override - never reset if user has manually adjusted
  if (manualBrightness) {
    return; // respect manual override - don't change brightness
  }
  if (charging) {
    setBrightness(BRIGHTNESS_HIGH);
  } else {
    setBrightness(BRIGHTNESS_LOW);
  }
}

void Display::increaseBrightnessStep(int step) {
  // CRITICAL: Set manualBrightness FIRST before any brightness operations
  // This prevents handlePowerManagement from resetting brightness
  manualBrightness = true;

  // Read current brightness from member variable
  int currentBrightness = brightness;
  int newBrightness = currentBrightness + step;

  // Ensure brightness doesn't exceed maximum
  if (newBrightness > 255) {
    newBrightness = 255;
  }

  // Apply brightness change
  setBrightness(newBrightness);
  // Save brightness and manual flag to persist the change
  saveBrightness();
}

void Display::decreaseBrightnessStep(int step) {
  // CRITICAL: Set manualBrightness FIRST before any brightness operations
  // This prevents handlePowerManagement from resetting brightness
  manualBrightness = true;

  // Read current brightness from member variable
  int currentBrightness = brightness;
  int newBrightness = currentBrightness - step;

  // Ensure brightness doesn't go below minimum (at least 10 for visibility)
  if (newBrightness < 10) {
    newBrightness = 10;
  }

  // Apply brightness change
  setBrightness(newBrightness);
  // Save brightness and manual flag to persist the change
  saveBrightness();
}

void Display::saveBrightness() {
  Preferences prefs;
  if (prefs.begin("m5clock", false)) { // false = read-write mode
    prefs.putUChar("brightness", brightness);
    prefs.putBool("brightness_manual", manualBrightness);
    prefs.end();
  }
}

void Display::loadBrightness() {
  Preferences prefs;
  if (prefs.begin("m5clock", true)) { // true = read-only mode
    int savedBrightness = prefs.getUChar("brightness", BRIGHTNESS_HIGH);
    manualBrightness = prefs.getBool("brightness_manual", false);
    setBrightness(savedBrightness);
    prefs.end();
  } else {
    // If preferences failed, use default
    setBrightness(BRIGHTNESS_HIGH);
  }
}

void Display::showTimezoneSetting(int selection) {
  // Leaving clock view; force next clock draw to fully refresh
  initialClockDrawn = false;
  M5.Display.fillScreen(TFT_BLACK);
  
  M5.Display.setTextDatum(TL_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  
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
  
  // Calculate visible range - show 6 items, with selected item in the middle when possible
  int visibleCount = 6;
  int startIdx = max(0, selection - 2);  // Show 2 items before selection
  int endIdx = min(19, startIdx + visibleCount);
  
  // Adjust start if we're near the end
  if (endIdx - startIdx < visibleCount) {
    startIdx = max(0, endIdx - visibleCount);
  }
  
  // Start list below title (title at y=5, text size 2 is ~16px tall, so start at y=25)
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
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString("DST", screenWidth / 2, screenHeight / 2 - 30);
  
  // ON/OFF with highlight
  M5.Display.setFont(&fonts::FreeSans9pt7b);
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

void Display::showBrightnessSetting(int brightness) {
  // Leaving clock view; force next clock draw to fully refresh
  initialClockDrawn = false;
  M5.Display.fillScreen(TFT_BLACK);

  M5.Display.setTextDatum(MC_DATUM);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.drawString("Brightness", screenWidth / 2, 20);

  // Show brightness value (0-255)
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  char brightnessStr[16];
  snprintf(brightnessStr, sizeof(brightnessStr), "%d", brightness);
  M5.Display.drawString(brightnessStr, screenWidth / 2, screenHeight / 2 - 20);

  // Show percentage
  int percentage = (brightness * 100) / 255;
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  char percentStr[16];
  snprintf(percentStr, sizeof(percentStr), "%d%%", percentage);
  M5.Display.drawString(percentStr, screenWidth / 2, screenHeight / 2 + 20);

  // Show instructions
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  M5.Display.drawString("Button A: Increase", screenWidth / 2, screenHeight - 40);
  M5.Display.drawString("Button B: Decrease", screenWidth / 2, screenHeight - 25);
  M5.Display.drawString("Button C: Back", screenWidth / 2, screenHeight - 10);
}

// Touchscreen interaction methods
char Display::getKeyboardKeyAt(int x, int y) {
  // On-screen keyboard layout (same as in showWiFiPassword)
  const char* keyboardRowsUpper[] = {
    "QWERTYUIOP",
    "ASDFGHJKL",
    "ZXCVBNM",
    "  \b\n"
  };
  const char* keyboardRowsLower[] = {
    "qwertyuiop",
    "asdfghjkl",
    "zxcvbnm",
    "  \b\n"
  };
  const char** keyboardRows = keyboardShift ? keyboardRowsLower : keyboardRowsUpper;
  
  int keyWidth = 28;
  int keyHeight = 30;
  int keySpacing = 2;
  int startY = 85;  // Match the keyboard position in showWiFiPassword
  int startX = 10;
  
  // Check if touch is in keyboard area
  if (y < startY || y > startY + 4 * (keyHeight + keySpacing)) {
    return 0;
  }
  
  int row = (y - startY) / (keyHeight + keySpacing);
  if (row < 0 || row >= 4) return 0;
  
  if (row == 3) {
    // Special keys row
    int rowStartX = startX;
    // Shift button
    if (x >= rowStartX && x < rowStartX + keyWidth * 2) {
      // Toggle shift state
      keyboardShift = !keyboardShift;
      // Redraw the keyboard by calling showWiFiPassword again
      // We'll need to get the current password from the view, but for now return a special code
      return 1; // Special code for shift toggle
    }
    // Space
    if (x >= rowStartX + keyWidth * 2 + keySpacing && x < rowStartX + keyWidth * 5 + keySpacing) {
      return ' ';
    }
    // Backspace
    if (x >= rowStartX + keyWidth * 5 + keySpacing * 2 && x < rowStartX + keyWidth * 7 + keySpacing * 2) {
      return '\b';
    }
    // Enter
    if (x >= rowStartX + keyWidth * 7 + keySpacing * 3 && x < rowStartX + keyWidth * 9 + keySpacing * 3) {
      return '\n';
    }
    return 0;
  }
  
  int rowLen = strlen(keyboardRows[row]);
  int rowStartX = startX;
  if (row == 1) rowStartX += keyWidth / 2;
  if (row == 2) rowStartX += keyWidth;
  
  int col = (x - rowStartX) / (keyWidth + keySpacing);
  if (col >= 0 && col < rowLen) {
    // Check if touch is actually within the key bounds
    int keyX = rowStartX + col * (keyWidth + keySpacing);
    if (x >= keyX && x < keyX + keyWidth) {
      return keyboardRows[row][col];
    }
  }
  
  return 0;
}

int Display::getTouchedItem(int x, int y, ViewItem view) {
  if (view == VIEW_MAIN_MENU) {
    // Main menu items: each item is ~26 pixels tall, starting at y=16
    int itemHeight = 26;
    int startY = 16;
    int itemIndex = (y - startY) / itemHeight;
    if (itemIndex >= 0 && itemIndex < 5 && y >= startY && x >= 10 && x < screenWidth - 10) {
      return itemIndex;
    }
  } else if (view == VIEW_WIFI_SELECT) {
    // WiFi list items: each item is ~22 pixels tall, starting at y=25
    int itemHeight = 22;
    int startY = 25;
    int itemIndex = (y - startY) / itemHeight;
    if (itemIndex >= 0 && itemIndex < 20 && y >= startY && x >= 10 && x < screenWidth - 10) {
      return itemIndex;
    }
  } else if (view == VIEW_TIMEZONE_SET) {
    // Timezone list items: each item is ~22 pixels tall, starting at y=25 (below title)
    int itemHeight = 22;
    int startY = 25;
    int itemIndex = (y - startY) / itemHeight;
    if (itemIndex >= 0 && itemIndex < 6 && y >= startY && x >= 10 && x < screenWidth - 10) {
      // Map visible index to actual timezone index
      // We need to get the current selection to calculate the visible range
      // For now, return -1 and let button navigation handle it
      // Touch selection will need to be handled differently
      return -1; // Disable touch selection for now, use buttons
    }
  }
  return -1;
}

bool Display::isBackButtonTouched(int x, int y) {
  // Back button is typically in top-left corner: 5,5 with size 50x25
  return (x >= 5 && x <= 55 && y >= 5 && y <= 30);
}

void Display::resetKeyboardShift() {
  keyboardShift = false;
}

