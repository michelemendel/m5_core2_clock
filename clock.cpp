#include "clock.h"

// Preferences namespace for storing clock settings
const char* Clock::PREFS_NAMESPACE = "m5clock";

Clock::Clock() {
  currentTime.hours = 0;
  currentTime.minutes = 0;
  currentTime.seconds = 0;
  alarm.hours = 0;
  alarm.minutes = 0;
  alarm.enabled = false;
  alarmRinging = false;
  alarmStartTime = 0;
  alarmEnabled = false;
  timezoneIndex = 10; // Default to UTC+2 (index 10)
  currentTimezone.offsetHours = 2;
  currentTimezone.daylightSaving = true;
  currentTimezone.name = "EET";
  dstCustomEnabled = false;
  dstCustomValue = true;
  alarmAutoShutoffSeconds = ALARM_DEFAULT_AUTO_SHUTOFF_SECONDS;
  alarmVolume = 128; // mid volume default
}

void Clock::setTime(uint8_t hours, uint8_t minutes, uint8_t seconds) {
  currentTime.hours = hours % 24;
  currentTime.minutes = minutes % 60;
  currentTime.seconds = seconds % 60;
}

void Clock::setTime(uint8_t hours, uint8_t minutes) {
  currentTime.hours = hours % 24;
  currentTime.minutes = minutes % 60;
  currentTime.seconds = 0;
}

Time Clock::getTime() const {
  return currentTime;
}

void Clock::tick() {
  currentTime.seconds++;
  if (currentTime.seconds >= 60) {
    currentTime.seconds = 0;
    currentTime.minutes++;
    if (currentTime.minutes >= 60) {
      currentTime.minutes = 0;
      currentTime.hours++;
      if (currentTime.hours >= 24) {
        currentTime.hours = 0;
      }
    }
  }
  
  // Update alarm timer if ringing
  if (alarmRinging) {
    updateAlarmTimer();
  }
}

void Clock::setAlarm(uint8_t hours, uint8_t minutes) {
  alarm.hours = hours % 24;
  alarm.minutes = minutes % 60;
  save(); // Persist alarm settings
}

void Clock::enableAlarm(bool enabled) {
  alarm.enabled = enabled;
  alarmEnabled = enabled;
  save(); // Persist alarm settings
}

Alarm Clock::getAlarm() const {
  return alarm;
}

bool Clock::isAlarmTime() const {
  if (!alarm.enabled) {
    return false;
  }
  
  return (currentTime.hours == alarm.hours && 
          currentTime.minutes == alarm.minutes && 
          currentTime.seconds == 0);
}

bool Clock::isAlarmEnabled() const {
  return alarm.enabled;
}

bool Clock::isAlarmRinging() const {
  return alarmRinging;
}

void Clock::startAlarm() {
  alarmRinging = true;
  alarmStartTime = millis();
}

void Clock::stopAlarm() {
  alarmRinging = false;
  alarmStartTime = 0;
}

void Clock::updateAlarmTimer() {
  if (alarmRinging) {
    unsigned long elapsed = (millis() - alarmStartTime) / 1000;
    uint16_t limit = alarmAutoShutoffSeconds == 0 ? ALARM_DEFAULT_AUTO_SHUTOFF_SECONDS : alarmAutoShutoffSeconds;
    if (elapsed >= limit) {
      stopAlarm();
    }
  }
}

unsigned long Clock::getAlarmStartTime() const {
  return alarmStartTime;
}

void Clock::setTimezone(int8_t offsetHours, bool daylightSaving, const String& name) {
  currentTimezone.offsetHours = offsetHours;
  currentTimezone.daylightSaving = daylightSaving;
  currentTimezone.name = name;
}

Timezone Clock::getTimezone() const {
  return currentTimezone;
}

void Clock::setTimezoneIndex(int8_t index) {
  if (index >= 0 && index < TIMEZONE_COUNT) {
    timezoneIndex = index;
    // Update current timezone based on index
    currentTimezone.offsetHours = getTimezoneOffset(index);
    // Respect user DST override if enabled, otherwise default
    currentTimezone.daylightSaving = dstCustomEnabled ? dstCustomValue : getTimezoneDST(index);
    currentTimezone.name = getTimezoneName(index);
    save(); // Persist timezone settings
  }
}

int8_t Clock::getTimezoneIndex() const {
  return timezoneIndex;
}

int8_t Clock::getTimezoneCount() const {
  return TIMEZONE_COUNT;
}

String Clock::getTimezoneName(int8_t index) const {
  const char* names[] = {
    "HST",  // -10 Hawaii
    "AKST", // -9 Alaska
    "PST",  // -8 Pacific
    "MST",  // -7 Mountain
    "CST",  // -6 Central
    "EST",  // -5 Eastern
    "AST",  // -4 Atlantic
    "BRT",  // -3 Brazil
    "GMT",  // 0 Greenwich
    "CET",  // +1 Central Europe
    "EET",  // +2 Eastern Europe
    "MSK",  // +3 Moscow
    "GST",  // +4 Gulf Standard Time
    "PKT",  // +5 Pakistan
    "BST",  // +6 Bangladesh
    "ICT",  // +7 Indochina
    "CST",  // +8 China
    "JST",  // +9 Japan
    "AEST"  // +10 Australia
  };
  
  if (index >= 0 && index < TIMEZONE_COUNT) {
    return String(names[index]);
  }
  return "UTC";
}

int8_t Clock::getTimezoneOffset(int8_t index) const {
  int8_t offsets[] = {
    -10, // HST
    -9,  // AKST
    -8,  // PST
    -7,  // MST
    -6,  // CST
    -5,  // EST
    -4,  // AST
    -3,  // BRT
    0,   // GMT
    +1,  // CET
    +2,  // EET
    +3,  // MSK
    +4,  // GST
    +5,  // PKT
    +6,  // BST
    +7,  // ICT
    +8,  // CST
    +9,  // JST
    +10  // AEST
  };
  
  if (index >= 0 && index < TIMEZONE_COUNT) {
    return offsets[index];
  }
  return 0;
}

bool Clock::getTimezoneDST(int8_t index) const {
  // Most timezones use DST except some
  bool dstEnabled[] = {
    false, // HST - Hawaii doesn't use DST
    true,  // AKST - Alaska uses DST
    true,  // PST - Pacific uses DST
    true,  // MST - Mountain uses DST
    true,  // CST - Central uses DST
    true,  // EST - Eastern uses DST
    true,  // AST - Atlantic uses DST
    false, // BRT - Brazil doesn't use DST
    true,  // GMT - Greenwich uses DST (BST)
    true,  // CET - Central Europe uses DST
    true,  // EET - Eastern Europe uses DST
    false, // MSK - Russia doesn't use DST
    false, // GST - Gulf doesn't use DST
    false, // PKT - Pakistan doesn't use DST
    false, // BST - Bangladesh doesn't use DST
    false, // ICT - Indochina doesn't use DST
    false, // CST - China doesn't use DST
    false, // JST - Japan doesn't use DST
    true   // AEST - Australia uses DST
  };
  
  if (index >= 0 && index < TIMEZONE_COUNT) {
    return dstEnabled[index];
  }
  return false;
}

void Clock::load() {
  Preferences prefs;
  if (!prefs.begin(PREFS_NAMESPACE, true)) { // true = read-only mode
    // Failed to open preferences - use defaults
    return;
  }
  
  // Load alarm settings
  alarm.hours = prefs.getUChar("alarm_h", 0);
  alarm.minutes = prefs.getUChar("alarm_m", 0);
  alarm.enabled = prefs.getBool("alarm_en", false);
  alarmEnabled = alarm.enabled;
  // Load last known time (optional)
  currentTime.hours = prefs.getUChar("time_h", currentTime.hours);
  currentTime.minutes = prefs.getUChar("time_m", currentTime.minutes);
  currentTime.seconds = prefs.getUChar("time_s", currentTime.seconds);
  alarmAutoShutoffSeconds = prefs.getUShort("alarm_len", ALARM_DEFAULT_AUTO_SHUTOFF_SECONDS);
  alarmVolume = prefs.getUChar("alarm_vol", 128);
  
  // Load timezone index
  timezoneIndex = prefs.getChar("tz_idx", 10); // Default to UTC+2 (index 10)
  if (timezoneIndex < 0 || timezoneIndex >= TIMEZONE_COUNT) {
    timezoneIndex = 10; // Reset to valid default if corrupted
  }

  // Load DST user override
  dstCustomEnabled = prefs.getBool("dst_custom", false);
  dstCustomValue = prefs.getBool("dst_val", true);
  
  // Update current timezone based on loaded index
  currentTimezone.offsetHours = getTimezoneOffset(timezoneIndex);
  currentTimezone.daylightSaving = dstCustomEnabled ? dstCustomValue : getTimezoneDST(timezoneIndex);
  currentTimezone.name = getTimezoneName(timezoneIndex);
  
  prefs.end();
}

void Clock::save() {
  Preferences prefs;
  if (!prefs.begin(PREFS_NAMESPACE, false)) { // false = read-write mode
    // Failed to open preferences - settings won't be saved
    return;
  }
  
  // Save alarm settings
  prefs.putUChar("alarm_h", alarm.hours);
  prefs.putUChar("alarm_m", alarm.minutes);
  prefs.putBool("alarm_en", alarm.enabled);
  // Persist current time snapshot
  prefs.putUChar("time_h", currentTime.hours);
  prefs.putUChar("time_m", currentTime.minutes);
  prefs.putUChar("time_s", currentTime.seconds);
  prefs.putUShort("alarm_len", alarmAutoShutoffSeconds);
  prefs.putUChar("alarm_vol", alarmVolume);
  
  // Save timezone index
  prefs.putChar("tz_idx", timezoneIndex);
  // Save DST user override
  prefs.putBool("dst_custom", dstCustomEnabled);
  prefs.putBool("dst_val", dstCustomValue);
  
  prefs.end();
}

void Clock::setTimezoneDST(bool enabled) {
  dstCustomEnabled = true;
  dstCustomValue = enabled;
  currentTimezone.daylightSaving = enabled;
  save();
}

bool Clock::isDSTCustom() const {
  return dstCustomEnabled;
}

bool Clock::getDSTCustomValue() const {
  return dstCustomValue;
}

void Clock::setAlarmAutoShutoffSeconds(uint16_t seconds) {
  alarmAutoShutoffSeconds = seconds;
  save();
}

uint16_t Clock::getAlarmAutoShutoffSeconds() const {
  return alarmAutoShutoffSeconds;
}

void Clock::setAlarmVolume(uint8_t volume) {
  alarmVolume = volume;
  save();
}

uint8_t Clock::getAlarmVolume() const {
  return alarmVolume;
}

