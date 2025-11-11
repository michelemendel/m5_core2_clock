#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>
#include <Preferences.h>

// Default configuration constants
#define ALARM_DEFAULT_AUTO_SHUTOFF_SECONDS 60

struct Time {
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
};

struct Alarm {
  uint8_t hours;
  uint8_t minutes;
  bool enabled;
};

struct Timezone {
  int8_t offsetHours;  // -12 to +14
  bool daylightSaving; // true if DST is enabled
  String name;         // Display name like "EST", "PST", "CET"
};

class Clock {
public:
  Clock();
  
  // Time management
  void setTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
  void setTime(uint8_t hours, uint8_t minutes);
  Time getTime() const;
  void tick(); // Increment time by 1 second
  
  // Alarm management
  void setAlarm(uint8_t hours, uint8_t minutes);
  void enableAlarm(bool enabled);
  Alarm getAlarm() const;
  bool isAlarmTime() const;
  bool isAlarmEnabled() const;
  
  // Alarm state
  bool isAlarmRinging() const;
  void startAlarm();
  void stopAlarm();
  void updateAlarmTimer(); // Called every second to handle auto-shutoff
  unsigned long getAlarmStartTime() const; // Get alarm start time for timing
  
  // Alarm options
  void setAlarmAutoShutoffSeconds(uint16_t seconds);
  uint16_t getAlarmAutoShutoffSeconds() const;
  void setAlarmVolume(uint8_t volume); // 0-255
  uint8_t getAlarmVolume() const;
  
  // Timezone management
  void setTimezone(int8_t offsetHours, bool daylightSaving, const String& name);
  Timezone getTimezone() const;
  void setTimezoneIndex(int8_t index);
  int8_t getTimezoneIndex() const;
  int8_t getTimezoneCount() const;
  String getTimezoneName(int8_t index) const;
  int8_t getTimezoneOffset(int8_t index) const;
  bool getTimezoneDST(int8_t index) const;
  void setTimezoneDST(bool enabled); // User override DST for current timezone
  bool isDSTCustom() const;          // Whether user override is active
  bool getDSTCustomValue() const;    // Current DST override value
  
  // Persistence
  void load(); // Load settings from flash memory
  void save(); // Save settings to flash memory
  
private:
  Time currentTime;
  Alarm alarm;
  bool alarmRinging;
  unsigned long alarmStartTime;
  bool alarmEnabled;
  Timezone currentTimezone;
  int8_t timezoneIndex;
  static const int8_t TIMEZONE_COUNT = 19;
  
  // Preferences namespace for persistent storage
  static const char* PREFS_NAMESPACE;

  // DST user override persistence
  bool dstCustomEnabled;
  bool dstCustomValue;

  // Alarm options persistence
  uint16_t alarmAutoShutoffSeconds; // seconds until auto stop
  uint8_t alarmVolume; // 0-255
};

#endif // CLOCK_H

