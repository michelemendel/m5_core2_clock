/*
 * M5Core2 Clock Application
 * 
 * For usage instructions and documentation, see README.md
 */

#include <M5Unified.h>
#include <Preferences.h>
#include <esp_sleep.h>
#include "clock.h"
#include "display.h"
#include "connectivity.h"
#include "io.h"
#include "view.h"

// Module instances
Clock appClock;
Display display;
Connectivity connectivity;
IO io;
View view;

// State variables
unsigned long lastSecond = 0;
unsigned long lastNTPCheck = 0;
unsigned long lastInputTime = 0;
unsigned long lastAlarmBeep = 0;  // Track last alarm beep time
unsigned long startupMillis = 0;   // For deferred WiFi connect
bool wifiConnectPending = false;   // Defer WiFi connect to after first render
bool wifiConnectHandled = false;   // UI updated after connect finishes
ViewItem lastViewState = VIEW_CLOCK;  // Track last view state to prevent unnecessary redraws
Time lastDisplayedTime = {0, 0, 0};   // Track last displayed time to prevent flicker
String wifiSSIDs[20];
int wifiSSIDCount = 0;
String wifiPassword = "";
bool wifiScanning = false;
bool wifiConnecting = false;
static const unsigned long SLEEP_DELAY_MS = 30000; // Sleep after 30 seconds of no input

// Saved WiFi credentials (loaded from Preferences)
String savedWifiSSID = "";
String savedWifiPassword = "";

static const char* PREFS_NS_APP = "m5clock";

void saveWiFiCredentials(const String& ssid, const String& password) {
  Preferences prefs;
  if (prefs.begin(PREFS_NS_APP, false)) {
    prefs.putString("wifi_ssid", ssid);
    prefs.putString("wifi_pwd", password);
    prefs.end();
  }
  savedWifiSSID = ssid;
  savedWifiPassword = password;
}

void loadWiFiCredentials() {
  Preferences prefs;
  if (prefs.begin(PREFS_NS_APP, true)) {
    savedWifiSSID = prefs.getString("wifi_ssid", "");
    savedWifiPassword = prefs.getString("wifi_pwd", "");
    prefs.end();
  }
}

// TEMP: Clear only the saved WiFi password to test startup behavior without auto-connect
// Comment out this function call in setup() when no longer needed
void clearSavedWiFiPassword() {
  Preferences prefs;
  if (prefs.begin(PREFS_NS_APP, false)) {
    prefs.putString("wifi_pwd", "");
    prefs.end();
  }
  savedWifiPassword = "";
}

// WiFi credentials are loaded/saved via Preferences; no hardcoded defaults

void setup() {
  // Configure M5 settings for Core2
  auto cfg = M5.config();
  cfg.clear_display = true;
  cfg.output_power = true;  // Required for screen backlight and peripherals
  
  // Initialize M5Core2
  M5.begin(cfg);
  
  // Initialize speaker for alarm
  M5.Speaker.begin();
  M5.Speaker.setVolume(128);  // Temporary default; will be overridden after load
  
  // Serial still available but not needed for keyboard input anymore
  Serial.begin(115200);  // Serial for debugging (optional)
  
  // Initialize modules
  display.begin();
  io.begin();
  connectivity.begin();
  
  // Load saved settings from flash memory (alarm, timezone, etc.)
  appClock.load();
  // Apply saved alarm volume
  M5.Speaker.setVolume(appClock.getAlarmVolume());
  
  // Use loaded/saved time; if NTP later succeeds, it will update
  lastDisplayedTime = appClock.getTime();
  
  // Load saved brightness (already done in display.begin())
  // Set initial brightness based on charger status (only if not manually set)
  display.setBrightnessForCharger(io.isCharging());
  
  // Initialize timers before first draw
  lastSecond = millis();
  // Immediately show clock with '...' for WiFi while connecting
  display.setWifiStatusOverride("...");
  display.showClock(appClock.getTime(), false, appClock.getAlarm(), appClock.getTimezone());
  
  // Load saved WiFi credentials
  loadWiFiCredentials();
  // TEMP: remove persisted WiFi password to simulate first-time WiFi entry
  // clearSavedWiFiPassword();
  if (savedWifiSSID.length() > 0) {
    // Defer WiFi connect until after first loop iterations
    wifiConnectPending = true;
    wifiConnectHandled = false;
  } else {
    // No saved SSID: show No WiFi immediately
    display.clearWifiStatusOverride();
    display.showClock(appClock.getTime(), false, appClock.getAlarm(), appClock.getTimezone());
  }
  
  startupMillis = millis();
  lastSecond = startupMillis;
  lastNTPCheck = millis();
  lastInputTime = millis();
  lastViewState = VIEW_CLOCK;
}

void loop() {
  // Update all modules
  updateModules();
  
  // Handle power management
  handlePowerManagement();
  
  // Process input events
  InputEvent event = processInputEvents();
  
  // Update clock and alarms
  updateClockAndAlarms();
  
  // Handle NTP sync
  handleNTPSync();

  // Deferred WiFi connect after initial render so clock starts immediately
  if (wifiConnectPending && (millis() - startupMillis > 1500)) {
    wifiConnectPending = false;
    // Start non-blocking connect; update() will progress it
    connectivity.beginConnect(savedWifiSSID, savedWifiPassword, 15000);
    // Clear override and redraw clock to reflect final WiFi status
    display.clearWifiStatusOverride();
    display.showClock(appClock.getTime(), false, appClock.getAlarm(), appClock.getTimezone());
  }

  // If non-blocking connect finished, update UI and allow NTP to sync via regular path
  if (connectivity.connectFinished() && !wifiConnectHandled) {
    display.showClock(appClock.getTime(), connectivity.isTimeSynced(), appClock.getAlarm(), appClock.getTimezone());
    wifiConnectHandled = true;
  }
  
  // Render display if needed
  updateDisplay(event);
  
  delay(10); // Small delay to prevent tight loop
}

void updateModules() {
  // M5.update() must be called to poll buttons and touchscreen
  M5.update();
  io.update();
  connectivity.update();
}

void handlePowerManagement() {
  bool charging = io.isCharging();
  display.setBrightnessForCharger(charging);
  
  // If not charging and no input for a while, handle sleep mode
  if (!charging && (millis() - lastInputTime > SLEEP_DELAY_MS)) {
    // No input for a while - enter light sleep
    // ESP32 will wake on button/key press automatically
    // Note: This is a simplified implementation - actual Cardputer may need GPIO wake config
    delay(100); // Small delay before sleep
    return;
  }
}

InputEvent processInputEvents() {
  InputEvent event = io.getInputEvent();
  
  // If alarm is ringing, any key/button stops it
  if (appClock.isAlarmRinging() && event != INPUT_NONE) {
    appClock.stopAlarm();
    M5.Speaker.stop();  // Stop alarm sound
    io.clearInput();
    return INPUT_NONE;  // Consume this event, don't process as normal input
  }
  
  // Update last input time if there's input
  if (event != INPUT_NONE) {
    lastInputTime = millis();
  }
  
  // Handle different input events
  if (event == INPUT_BUTTON_UP) {
    handleButtonUp();
    io.clearInput();
  } else if (event == INPUT_BUTTON_DOWN) {
    handleButtonDown();
    io.clearInput();
  } else if (event == INPUT_BUTTON_SELECT) {
    handleViewSelect();
    io.clearInput();
  } else if (event == INPUT_BUTTON_BACK) {
    handleBack();
    io.clearInput();
  } else if (event == INPUT_KEY_PRESSED) {
    handleKeyPressed();
  }
  
  // Check for touchscreen input (only if no button event occurred)
  // Also double-check that no buttons are currently pressed to prevent conflicts
  // CRITICAL: Only process touch if we have NO button event AND no buttons are pressed
  if (event == INPUT_NONE) {
    // Triple-check: don't process touch if any button is currently pressed
    bool anyButtonPressed = M5.BtnA.isPressed() || M5.BtnB.isPressed() || M5.BtnC.isPressed();
    if (!anyButtonPressed) {
      int touchX = io.getTouchX();
      int touchY = io.getTouchY();
      // Only process touch if coordinates are valid AND we still have no button event
      // (double-check event in case it changed)
      if (touchX >= 0 && touchY >= 0 && io.getInputEvent() == INPUT_NONE) {
        handleTouchInput(touchX, touchY);
        io.clearInput();
      }
    } else {
      // If any button is pressed, clear touch coordinates to be safe
      io.clearInput();
    }
  }
  
  return event;
}

void handleButtonUp() {
  ViewItem state = view.getState();
  if (state == VIEW_WIFI_PASSWORD) {
    // In password entry, ignore Up (do not go back)
  } else if (state == VIEW_CLOCK) {
    // In clock view, Up increases brightness
    display.increaseBrightnessStep(10);
  } else {
    view.navigateUp();
    // Force redraw for views that need it - get state after navigation
    state = view.getState();
    if (state == VIEW_TIMEZONE_SET) {
      display.showTimezoneSetting(view.getTimezoneSelection());
    } else if (state == VIEW_WIFI_SCAN || state == VIEW_WIFI_SELECT) {
      display.showWiFiScan(wifiSSIDs, wifiSSIDCount, view.getWiFiSelection());
    }
  }
}

void handleButtonDown() {
  ViewItem state = view.getState();
  if (state == VIEW_WIFI_PASSWORD) {
    // In password entry, ignore Down (do not go back)
  } else if (state == VIEW_CLOCK) {
    // In clock view, Down decreases brightness
    display.decreaseBrightnessStep(10);
  } else {
    view.navigateDown();
    // Force redraw for views that need it - get state after navigation
    state = view.getState();
    if (state == VIEW_TIMEZONE_SET) {
      display.showTimezoneSetting(view.getTimezoneSelection());
    } else if (state == VIEW_WIFI_SCAN || state == VIEW_WIFI_SELECT) {
      display.showWiFiScan(wifiSSIDs, wifiSSIDCount, view.getWiFiSelection());
    }
  }
}

void handleBack() {
  // Handle back navigation (same logic as 'q' key from keyboard)
  ViewItem prev = view.getState();
  if (prev == VIEW_WIFI_PASSWORD) {
    // Leaving password view, disable text input mode and clear pwd
    io.setTextInputMode(false);
  }
  if (prev == VIEW_TIME_SET) {
    // Save manual time setting when going back
    appClock.setTime(view.getTimeHours(), view.getTimeMinutes());
  }
  if (prev == VIEW_TIMEZONE_SET) {
    // Apply timezone and DST
    Timezone oldTz = appClock.getTimezone();
    int8_t oldTotal = oldTz.offsetHours + (oldTz.daylightSaving ? 1 : 0);
    int newIndex = view.getTimezoneSelection();
    appClock.setTimezoneIndex(newIndex);
    appClock.setTimezoneDST(view.getTimezoneDST());
    Timezone newTz = appClock.getTimezone();
    int8_t newTotal = newTz.offsetHours + (newTz.daylightSaving ? 1 : 0);

    // Prefer NTP sync if WiFi connected; fallback to local hour delta
    if (connectivity.isConnected()) {
      display.showSyncing();
      if (connectivity.syncTime(newTz.offsetHours, newTz.daylightSaving)) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
          appClock.setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        }
      }
    } else {
      int delta = newTotal - oldTotal;
      if (delta != 0) {
        Time t = appClock.getTime();
        int hours = (int)t.hours + delta;
        while (hours < 0) hours += 24;
        while (hours >= 24) hours -= 24;
        appClock.setTime((uint8_t)hours, t.minutes, t.seconds);
      }
    }
  }
  view.back();
  // After view.back(), the view state changes, but if we showed syncing,
  // explicitly redraw the appropriate view to avoid getting stuck
  if (view.getState() == VIEW_MAIN_MENU) {
    display.showMainView(view.getMainViewSelection());
  } else if (view.getState() == VIEW_CLOCK) {
    Time time = appClock.getTime();
    bool wifiConnected = connectivity.isTimeSynced();
    Alarm alarm = appClock.getAlarm();
    display.showClock(time, wifiConnected, alarm, appClock.getTimezone());
  }
  if (view.getState() == VIEW_WIFI_PASSWORD) {
    wifiPassword = "";
  }
}

void handleTouchInput(int x, int y) {
  // Process touchscreen input based on current view
  ViewItem state = view.getState();
  
  if (state == VIEW_WIFI_PASSWORD) {
    // Handle on-screen keyboard input
    char key = display.getKeyboardKeyAt(x, y);
    if (key == 1) {
      // Special code for shift toggle - redraw the keyboard
      display.showWiFiPassword(wifiSSIDs[view.getWiFiSelection()], wifiPassword);
    } else if (key != 0) {
      handleKeyInput(key);
    }
  } else {
    // Handle menu selection and back button
    int selectedItem = display.getTouchedItem(x, y, state);
    if (selectedItem >= 0) {
      // Item was selected
      if (state == VIEW_MAIN_MENU) {
        view.setMainViewSelection(selectedItem);
        handleViewSelect();
      } else if (state == VIEW_WIFI_SELECT) {
        view.setWiFiSelection(selectedItem);
        handleViewSelect();
      } else if (state == VIEW_TIMEZONE_SET) {
        // Touch selection disabled for timezone - use buttons instead
        // view.setTimezoneSelection(selectedItem);
      }
    } else if (state == VIEW_CLOCK) {
      // Tap on clock view opens main menu
      handleViewSelect();
    }
  }
}

void handleKeyPressed() {
  char key = io.getLastKey();
  // In WiFi password view, treat all printable keys as text (no back/navigation)
  if (view.getState() == VIEW_WIFI_PASSWORD) {
    handleKeyInput(key);
    io.clearInput();
    return;
  }
  // Check for back key (legacy keyboard support)
  if (key == 'q' || key == 'Q') {  // 'q' or 'Q' for back
    // Legacy keyboard back - use handleBack() function
    handleBack();
    io.clearInput();
  } else {
    handleKeyInput(key);
    io.clearInput();
  }
}

void updateClockAndAlarms() {
  // Update clock every second
  unsigned long currentMillis = millis();
  if (currentMillis - lastSecond >= 1000) {
    lastSecond = currentMillis;
    appClock.tick();
    
    // Check if alarm should trigger
    if (appClock.isAlarmTime() && !appClock.isAlarmRinging()) {
      appClock.startAlarm();
      // Start alarm sound (beep pattern)
      M5.Speaker.setVolume(appClock.getAlarmVolume());
      M5.Speaker.tone(800, 200);  // 800Hz for 200ms
      lastAlarmBeep = currentMillis;  // Initialize beep timer
    }
  }
  
  // If alarm is ringing, continue beeping every second
  // This needs to run every loop iteration, not just once per second
  if (appClock.isAlarmRinging()) {
    unsigned long currentMillis = millis();
    // Beep every second (1000ms intervals)
    if (currentMillis - lastAlarmBeep >= 1000) {
      M5.Speaker.setVolume(appClock.getAlarmVolume());
      M5.Speaker.tone(800, 200);  // 800Hz beep for 200ms
      lastAlarmBeep = currentMillis;
    }
  } else {
    // Reset beep timer when alarm is not ringing
    lastAlarmBeep = 0;
  }
}

void handleNTPSync() {
  // Check NTP sync interval
  if (connectivity.shouldSync()) {
    display.showSyncing();
    Timezone currentTz = appClock.getTimezone();
    if (connectivity.syncTime(currentTz.offsetHours, currentTz.daylightSaving)) {
      // Update clock from NTP
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        appClock.setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      }
    }
    // Always redraw clock view after sync attempt (whether success or failure)
    // This ensures we don't get stuck on "Syncing time..." if sync fails
    Time time = appClock.getTime();
    bool wifiConnected = connectivity.isTimeSynced();
    Alarm alarm = appClock.getAlarm();
    display.showClock(time, wifiConnected, alarm, currentTz);
  }
}

void updateDisplay(InputEvent event) {
  // Render display only when view state or time changes
  ViewItem currentViewState = view.getState();
  Time currentTime = appClock.getTime();
  
  // Check if we need to redraw
  bool needsRedraw = false;
  if (currentViewState != lastViewState) {
    needsRedraw = true;
    lastViewState = currentViewState;
  } else if (currentViewState == VIEW_CLOCK) {
    // For clock view, only redraw if seconds changed
    if (currentTime.seconds != lastDisplayedTime.seconds) {
      needsRedraw = true;
      lastDisplayedTime = currentTime;
    }
  } else if (currentViewState == VIEW_WIFI_PASSWORD) {
    // For password view, always redraw on any input (to show password length changes)
    needsRedraw = (event != INPUT_NONE);
    // Also redraw if password changed (catch all cases)
    static String lastPassword = "";
    if (wifiPassword != lastPassword) {
      needsRedraw = true;
      lastPassword = wifiPassword;
    }
  } else {
    // For other views, check if any input event occurred
    needsRedraw = (event != INPUT_NONE);
  }
  
  if (needsRedraw) {
    renderDisplay();
  }
}

void handleViewSelect() {
  ViewItem state = view.getState();
  
  switch (state) {
    case VIEW_CLOCK:
      view.setState(VIEW_MAIN_MENU);
      break;
      
    case VIEW_MAIN_MENU:
      {
        MenuItem selection = view.getMainViewSelection();
        if (selection == MENU_ITEM_WIFI) {
          // WiFi Setup
          view.setState(VIEW_WIFI_SCAN);
          wifiScanning = true;
          wifiSSIDCount = 0;
        } else if (selection == MENU_ITEM_ALARM) {
          // Set Alarm
          view.setState(VIEW_ALARM_SET);
          Alarm currentAlarm = appClock.getAlarm();
          view.setAlarmHours(currentAlarm.hours);
          view.setAlarmMinutes(currentAlarm.minutes);
          view.setAlarmEnabled(currentAlarm.enabled);
          view.setAlarmLengthSeconds(appClock.getAlarmAutoShutoffSeconds());
          view.setAlarmVolume(appClock.getAlarmVolume());
        } else if (selection == MENU_ITEM_TIME) {
          // Set Time
          view.setState(VIEW_TIME_SET);
          Time currentTime = appClock.getTime();
          view.setTimeHours(currentTime.hours);
          view.setTimeMinutes(currentTime.minutes);
          view.setTimeSettingField(0);
        } else if (selection == MENU_ITEM_TIMEZONE) {
          // Set Timezone
          view.setState(VIEW_TIMEZONE_SET);
          view.setTimezoneSelection(appClock.getTimezoneIndex());
          view.setTimezoneDST(appClock.getTimezone().daylightSaving);
        } else if (selection == MENU_ITEM_DST) {
          view.setState(VIEW_DST_SET);
          view.setTimezoneDST(appClock.getTimezone().daylightSaving);
        }
      }
      break;
      
    case VIEW_WIFI_SCAN:
      // Start scanning
      wifiScanning = true;
      break;
      
    case VIEW_WIFI_SELECT:
      view.setState(VIEW_WIFI_PASSWORD);
      wifiPassword = "";
      io.setTextInputMode(true);
      display.resetKeyboardShift(); // Reset to uppercase when entering password view
      break;
      
    case VIEW_WIFI_PASSWORD:
      // Password entry is handled in handleKeyInput
      // Select button also confirms password and connects (same as Enter key)
      if (wifiPassword.length() > 0) {
        wifiConnecting = true;
        display.showConnecting();
        if (connectivity.connectWiFi(wifiSSIDs[view.getWiFiSelection()], wifiPassword)) {
          // Brief success; immediately return to clock
          // Save credentials for quick reconnect next time
          saveWiFiCredentials(wifiSSIDs[view.getWiFiSelection()], wifiPassword);
          // Try initial NTP sync with current timezone
          Timezone currentTz = appClock.getTimezone();
          if (connectivity.syncTime(currentTz.offsetHours, currentTz.daylightSaving)) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
              appClock.setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            }
          }
          view.setState(VIEW_CLOCK);
        } else {
          display.showStatus("Failed!");
          delay(2000);
          view.setState(VIEW_WIFI_SELECT);
        }
        wifiConnecting = false;
      }
      break;
      
    case VIEW_TIME_SET:
      {
        int field = view.getTimeSettingField();
        if (field == 0) {
          // Advance to minutes
          view.select();
        } else if (field == 1) {
          // Advance to NTP sync option
          view.select();
        } else if (field == 2) {
          // Trigger NTP sync now
          display.showSyncing();
          Timezone tz = appClock.getTimezone();
          if (connectivity.isConnected() && connectivity.syncTime(tz.offsetHours, tz.daylightSaving)) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
              appClock.setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
              // Update the manual-set fields to the synced time
              view.setTimeHours((uint8_t)timeinfo.tm_hour);
              view.setTimeMinutes((uint8_t)timeinfo.tm_min);
            }
          }
          // After NTP sync, return to clock view
          view.setState(VIEW_CLOCK);
          Time time = appClock.getTime();
          bool wifiConnected = connectivity.isTimeSynced();
          Alarm alarm = appClock.getAlarm();
          display.showClock(time, wifiConnected, alarm, appClock.getTimezone());
        }
      }
      break;
      
    case VIEW_ALARM_SET:
      view.select();
      if (view.getAlarmSettingField() == 0) {
        // Finished setting alarm
        appClock.setAlarm(view.getAlarmHours(), view.getAlarmMinutes());
        appClock.enableAlarm(view.getAlarmEnabled());
        appClock.setAlarmAutoShutoffSeconds(view.getAlarmLengthSeconds());
        appClock.setAlarmVolume(view.getAlarmVolume());
        // Apply volume immediately
        M5.Speaker.setVolume(appClock.getAlarmVolume());
        view.setState(VIEW_CLOCK);
      }
      break;
      
    case VIEW_TIMEZONE_SET:
      // Apply timezone and return to main menu
      {
        Timezone oldTz = appClock.getTimezone();
        int8_t oldTotal = oldTz.offsetHours + (oldTz.daylightSaving ? 1 : 0);
        int newIndex = view.getTimezoneSelection();
        appClock.setTimezoneIndex(newIndex);
        appClock.setTimezoneDST(view.getTimezoneDST());
        Timezone newTz = appClock.getTimezone();
        int8_t newTotal = newTz.offsetHours + (newTz.daylightSaving ? 1 : 0);

        // Prefer NTP sync if WiFi connected; fallback to local hour delta
        if (connectivity.isConnected()) {
          display.showSyncing();
          if (connectivity.syncTime(newTz.offsetHours, newTz.daylightSaving)) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
              appClock.setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            }
          }
        } else {
          int delta = newTotal - oldTotal;
          if (delta != 0) {
            Time t = appClock.getTime();
            int hours = (int)t.hours + delta;
            while (hours < 0) hours += 24;
            while (hours >= 24) hours -= 24;
            appClock.setTime((uint8_t)hours, t.minutes, t.seconds);
          }
        }
        view.setState(VIEW_MAIN_MENU);
        display.showMainView(view.getMainViewSelection());
      }
      break;
    
    case VIEW_DST_SET:
      // Confirm DST toggle and apply, then return to clock
      {
        Timezone before = appClock.getTimezone();
        bool newDst = view.getTimezoneDST();
        // Apply DST flag
        appClock.setTimezoneDST(newDst);
        Timezone after = appClock.getTimezone();
        // Prefer NTP sync; fallback to +/-1 hour change
        if (connectivity.isConnected()) {
          display.showSyncing();
          if (connectivity.syncTime(after.offsetHours, after.daylightSaving)) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
              appClock.setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            }
          }
        } else if (before.daylightSaving != newDst) {
          int delta = newDst ? 1 : -1;
          Time t = appClock.getTime();
          int hours = (int)t.hours + delta;
          while (hours < 0) hours += 24;
          while (hours >= 24) hours -= 24;
          appClock.setTime((uint8_t)hours, t.minutes, t.seconds);
        }
        view.setState(VIEW_CLOCK);
        // Explicitly redraw clock view after sync to avoid getting stuck on "Syncing time..."
        Time time = appClock.getTime();
        bool wifiConnected = connectivity.isTimeSynced();
        Alarm alarm = appClock.getAlarm();
        display.showClock(time, wifiConnected, alarm, after);
      }
      break;
  }
}

void handleKeyInput(char key) {
  ViewItem state = view.getState();
  
  if (state == VIEW_WIFI_PASSWORD) {
    if (key == '\b' || key == 127) {
      // Backspace
      if (wifiPassword.length() > 0) {
        wifiPassword.remove(wifiPassword.length() - 1);
      }
    } else if (key == '\n' || key == '\r') {
      // Enter key - confirm password and connect
      if (wifiPassword.length() > 0) {
        wifiConnecting = true;
        display.showConnecting();
        if (connectivity.connectWiFi(wifiSSIDs[view.getWiFiSelection()], wifiPassword)) {
          // Brief success; immediately return to clock
          // Save credentials for quick reconnect next time
          saveWiFiCredentials(wifiSSIDs[view.getWiFiSelection()], wifiPassword);
          // Try initial NTP sync with current timezone
          Timezone currentTz = appClock.getTimezone();
          if (connectivity.syncTime(currentTz.offsetHours, currentTz.daylightSaving)) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
              appClock.setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            }
          }
          view.setState(VIEW_CLOCK);
        } else {
          display.showStatus("Failed!");
          delay(2000);
          view.setState(VIEW_WIFI_SELECT);
        }
        wifiConnecting = false;
      }
    } else if (key >= 32 && key <= 126) {
      // Printable character
      wifiPassword += key;
    }
  }
}

void renderDisplay() {
  ViewItem state = view.getState();
  
  switch (state) {
    case VIEW_CLOCK:
      {
        Time time = appClock.getTime();
        bool wifiConnected = connectivity.isTimeSynced();
        Alarm alarm = appClock.getAlarm();
        Timezone currentTz = appClock.getTimezone();
        
        display.showClock(time, wifiConnected, alarm, currentTz);
      }
      break;
      
    case VIEW_MAIN_MENU:
      display.showMainView(view.getMainViewSelection());
      break;
      
    case VIEW_WIFI_SCAN:
      if (wifiScanning) {
        wifiScanning = false;
        display.showStatus("Scanning...");
        connectivity.scanWiFi(wifiSSIDs, 20, wifiSSIDCount);
        view.setWiFiCount(wifiSSIDCount);
        if (wifiSSIDCount > 0) {
          view.setState(VIEW_WIFI_SELECT);
        }
      }
      display.showWiFiScan(wifiSSIDs, wifiSSIDCount, view.getWiFiSelection());
      break;
      
    case VIEW_WIFI_SELECT:
      display.showWiFiScan(wifiSSIDs, wifiSSIDCount, view.getWiFiSelection());
      break;
      
    case VIEW_WIFI_PASSWORD:
      display.showWiFiPassword(wifiSSIDs[view.getWiFiSelection()], wifiPassword);
      break;
      
    case VIEW_TIME_SET:
      display.showTimeSetting(
        view.getTimeHours(),
        view.getTimeMinutes(),
        view.getTimeSettingField()
      );
      break;
      
    case VIEW_ALARM_SET:
      display.showAlarmSetting(
        view.getAlarmHours(),
        view.getAlarmMinutes(),
        view.getAlarmEnabled(),
        view.getAlarmLengthSeconds(),
        view.getAlarmVolume(),
        view.getAlarmSettingField(),
        appClock.getTime()
      );
      break;
      
    case VIEW_TIMEZONE_SET:
      display.showTimezoneSetting(view.getTimezoneSelection());
      break;
    
    case VIEW_DST_SET:
      display.showDSTSetting(view.getTimezoneDST());
      break;
  }
}

