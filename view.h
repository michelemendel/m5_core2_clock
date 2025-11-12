#ifndef VIEW_H
#define VIEW_H

#include <Arduino.h>
#include <stdint.h>

enum ViewItem {
  VIEW_CLOCK,
  VIEW_MAIN_MENU,
  VIEW_WIFI_SCAN,
  VIEW_WIFI_SELECT,
  VIEW_WIFI_PASSWORD,
  VIEW_TIME_SET,
  VIEW_ALARM_SET,
  VIEW_TIMEZONE_SET,
  VIEW_DST_SET,
  VIEW_BRIGHTNESS_SET
};

enum MenuItem {
  MENU_ITEM_ALARM,
  MENU_ITEM_WIFI,
  MENU_ITEM_TIME,
  MENU_ITEM_TIMEZONE,
  MENU_ITEM_DST,
  MENU_ITEM_BRIGHTNESS
};

class View {
public:
  View();
  
  ViewItem getState() const;
  void setState(ViewItem state);
  
  // Navigation
  void navigateUp();
  void navigateDown();
  void select();
  void back();
  
  // Main menu
  MenuItem getMainViewSelection() const;
  void setMainViewSelection(int index);
  int getMainViewCount() const;
  
  // WiFi menu
  void setWiFiCount(int count);
  int getWiFiCount() const;
  int getWiFiSelection() const;
  void setWiFiSelection(int index);
  
  // Time setting menu
  int getTimeSettingField() const; // 0=hours, 1=minutes
  void setTimeSettingField(int field);
  void adjustTimeValue(int delta);
  uint8_t getTimeHours() const;
  uint8_t getTimeMinutes() const;
  void setTimeHours(uint8_t hours);
  void setTimeMinutes(uint8_t minutes);
  
  // Alarm setting menu
  int getAlarmSettingField() const; // 0=hours, 1=minutes, 2=enabled, 3=length(s), 4=volume
  void setAlarmSettingField(int field);
  void adjustAlarmValue(int delta);
  uint8_t getAlarmHours() const;
  uint8_t getAlarmMinutes() const;
  bool getAlarmEnabled() const;
  uint16_t getAlarmLengthSeconds() const;
  uint8_t getAlarmVolume() const;
  void setAlarmHours(uint8_t hours);
  void setAlarmMinutes(uint8_t minutes);
  void setAlarmEnabled(bool enabled);
  void setAlarmLengthSeconds(uint16_t seconds);
  void setAlarmVolume(uint8_t volume);
  
  // Timezone setting menu
  int getTimezoneSelection() const;
  void setTimezoneSelection(int index);
  int getTimezoneCount() const;
  bool getTimezoneDST() const;
  void setTimezoneDST(bool enabled);
  void toggleTimezoneDST();
  
private:
  ViewItem currentState;
  
  // Main menu
  MenuItem mainViewSelection;
  static const int MAIN_VIEW_COUNT = 6;
  
  // WiFi menu
  int wifiCount;
  int wifiSelection;
  
  // Time setting
  int timeSettingField;
  uint8_t timeHours;
  uint8_t timeMinutes;
  
  // Alarm setting
  int alarmSettingField;
  uint8_t alarmHours;
  uint8_t alarmMinutes;
  bool alarmEnabled;
  uint16_t alarmLengthSeconds;
  uint8_t alarmVolume;
  
  // Timezone setting
  int timezoneSelection;
  bool timezoneDST;
};

#endif // VIEW_H

