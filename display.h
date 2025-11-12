#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include "clock.h"
#include "view.h"

class Display {
public:
  Display();
  
  void begin();
  void update();
  
  void showClock(const Time& time, bool wifiConnected, const Alarm& alarm, const Timezone& timezone);
  void showMainView(int selection);
  void showWiFiScan(const String* ssids, int count, int selection);
  void showWiFiPassword(const String& ssid, const String& password);
  void showTimeSetting(uint8_t hours, uint8_t minutes, int field);
  void showAlarmSetting(uint8_t hours, uint8_t minutes, bool enabled, uint16_t lengthSeconds, uint8_t volume, int field, const Time& currentTime);
  void showTimezoneSetting(int selection);
  void showDSTSetting(bool enabled);
  void showBrightnessSetting(int brightness);
  void showStatus(const String& message);
  void showConnecting();
  void showSyncing();
  
  // Status overrides
  void setWifiStatusOverride(const String& label);
  void clearWifiStatusOverride();
  
  void setBrightness(int level);
  void setBrightnessForCharger(bool charging);
  void increaseBrightnessStep(int step = 10);
  void decreaseBrightnessStep(int step = 10);
  int getBrightness() const { return brightness; }
  void saveBrightness(); // Save brightness to Preferences
  void loadBrightness(); // Load brightness from Preferences
  
  // Touchscreen interaction methods
  char getKeyboardKeyAt(int x, int y);
  int getTouchedItem(int x, int y, ViewItem view);
  bool isBackButtonTouched(int x, int y);
  void resetKeyboardShift(); // Reset keyboard to uppercase
  
private:
  int screenWidth;
  int screenHeight;
  int brightness;
  static const int BRIGHTNESS_HIGH = 200;
  static const int BRIGHTNESS_LOW = 50;
  bool manualBrightness = false;
  // Cached state for partial redraws on the main clock view
  bool initialClockDrawn = false;
  bool lastWifiConnected = false;
  bool lastAlarmEnabled = false;
  uint8_t lastAlarmHours = 0;
  uint8_t lastAlarmMinutes = 0;
  String lastTimezoneDisplay = "";
  bool lastDSTOn = false;
  // Battery smoothing (5-sample moving average)
  int batterySamples[5] = {0, 0, 0, 0, 0};
  uint8_t batterySampleIndex = 0;
  uint8_t batterySampleCount = 0;
  int getSmoothedBatteryLevel(int rawLevel);
  // Heuristic for reliable charging display
  int prevSmoothLevel = -1;
  float prevVoltage = 0.0f;
  int chargingConfidence = 0; // increments when metrics rise, decrements otherwise
  // Debounced charging latch
  int chargeOnStreak = 0;
  int chargeOffStreak = 0;
  bool showChargingLatched = false;
  // Voltage trend streaks for robust charging detection
  int voltageRiseStreak = 0;
  int voltageNoRiseStreak = 0;

  // Optional status override (e.g., show '-' at startup)
  bool hasWifiOverride = false;
  String wifiOverride = "";
  // Cache for status line redraw decision
  bool lastHadWifiOverride = false;
  String lastWifiLabel = "";
  // Keyboard shift state (uppercase/lowercase)
  bool keyboardShift = false;
};

#endif // DISPLAY_H

