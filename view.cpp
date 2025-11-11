#include "view.h"

View::View() {
  currentState = VIEW_CLOCK;
  mainViewSelection = MENU_ITEM_ALARM;
  wifiCount = 0;
  wifiSelection = 0;
  timeSettingField = 0;
  timeHours = 0;
  timeMinutes = 0;
  alarmSettingField = 0;
  alarmHours = 0;
  alarmMinutes = 0;
  alarmEnabled = false;
  alarmLengthSeconds = 60;
  alarmVolume = 128;
  timezoneSelection = 10; // Default to UTC+2
  timezoneDST = true;
}

ViewItem View::getState() const {
  return currentState;
}

void View::setState(ViewItem state) {
  currentState = state;
}

void View::navigateUp() {
  switch (currentState) {
    case VIEW_MAIN_MENU:
      mainViewSelection = static_cast<MenuItem>((static_cast<int>(mainViewSelection) - 1 + MAIN_VIEW_COUNT) % MAIN_VIEW_COUNT);
      break;
    case VIEW_WIFI_SELECT:
      wifiSelection = (wifiSelection - 1 + wifiCount) % wifiCount;
      break;
    case VIEW_TIME_SET:
      if (timeSettingField == 0) {
        timeHours = (timeHours - 1 + 24) % 24;
      } else if (timeSettingField == 1) {
        timeMinutes = (timeMinutes - 1 + 60) % 60;
      } else {
        // field 2 is NTP option; no value change on up/down
      }
      break;
    case VIEW_ALARM_SET:
      if (alarmSettingField == 0) {
        alarmHours = (alarmHours - 1 + 24) % 24;
      } else if (alarmSettingField == 1) {
        alarmMinutes = (alarmMinutes - 1 + 60) % 60;
      } else if (alarmSettingField == 2) {
        alarmEnabled = !alarmEnabled;
      } else if (alarmSettingField == 3) {
        if (alarmLengthSeconds > 1) {
          alarmLengthSeconds -= 1;
        }
      } else if (alarmSettingField == 4) {
        if (alarmVolume > 0) alarmVolume -= 1;
      }
      break;
    case VIEW_TIMEZONE_SET:
      timezoneSelection = (timezoneSelection - 1 + 19) % 19;
      break;
    case VIEW_DST_SET:
      timezoneDST = !timezoneDST;
      break;
    default:
      break;
  }
}

void View::navigateDown() {
  switch (currentState) {
    case VIEW_MAIN_MENU:
      mainViewSelection = static_cast<MenuItem>((static_cast<int>(mainViewSelection) + 1) % MAIN_VIEW_COUNT);
      break;
    case VIEW_WIFI_SELECT:
      wifiSelection = (wifiSelection + 1) % wifiCount;
      break;
    case VIEW_TIME_SET:
      if (timeSettingField == 0) {
        timeHours = (timeHours + 1) % 24;
      } else if (timeSettingField == 1) {
        timeMinutes = (timeMinutes + 1) % 60;
      } else {
        // field 2 is NTP option; no value change on up/down
      }
      break;
    case VIEW_ALARM_SET:
      if (alarmSettingField == 0) {
        alarmHours = (alarmHours + 1) % 24;
      } else if (alarmSettingField == 1) {
        alarmMinutes = (alarmMinutes + 1) % 60;
      } else if (alarmSettingField == 2) {
        alarmEnabled = !alarmEnabled;
      } else if (alarmSettingField == 3) {
        if (alarmLengthSeconds < 600) {
          alarmLengthSeconds += 1;
        }
      } else if (alarmSettingField == 4) {
        if (alarmVolume < 255) alarmVolume += 1;
      }
      break;
    case VIEW_TIMEZONE_SET:
      timezoneSelection = (timezoneSelection + 1) % 19;
      break;
    case VIEW_DST_SET:
      timezoneDST = !timezoneDST;
      break;
    default:
      break;
  }
}

void View::select() {
  switch (currentState) {
    case VIEW_CLOCK:
      currentState = VIEW_MAIN_MENU;
      break;
    case VIEW_MAIN_MENU:
      if (mainViewSelection == MENU_ITEM_WIFI) {
        currentState = VIEW_WIFI_SCAN;
      } else if (mainViewSelection == MENU_ITEM_TIME) {
        currentState = VIEW_TIME_SET;
      } else if (mainViewSelection == MENU_ITEM_ALARM) {
        currentState = VIEW_ALARM_SET;
      } else if (mainViewSelection == MENU_ITEM_TIMEZONE) {
        currentState = VIEW_TIMEZONE_SET;
      } else if (mainViewSelection == MENU_ITEM_DST) {
        currentState = VIEW_DST_SET;
      }
      break;
    case VIEW_WIFI_SELECT:
      currentState = VIEW_WIFI_PASSWORD;
      break;
    case VIEW_TIME_SET:
      timeSettingField = (timeSettingField + 1) % 3;
      break;
    case VIEW_ALARM_SET:
      alarmSettingField = (alarmSettingField + 1) % 5;
      if (alarmSettingField == 0) {
        // Finished setting alarm
        currentState = VIEW_CLOCK;
      }
      break;
    case VIEW_TIMEZONE_SET:
      // Timezone selection is confirmed on ESC/back from controller
      // No action on Enter here
      break;
    case VIEW_DST_SET:
      // Confirm DST selection
      currentState = VIEW_CLOCK;
      break;
    default:
      break;
  }
}

void View::back() {
  switch (currentState) {
    case VIEW_MAIN_MENU:
      currentState = VIEW_CLOCK;
      break;
    case VIEW_WIFI_SCAN:
    case VIEW_WIFI_SELECT:
    case VIEW_WIFI_PASSWORD:
      currentState = VIEW_MAIN_MENU;
      break;
    case VIEW_TIME_SET:
      currentState = VIEW_MAIN_MENU;
      timeSettingField = 0;
      break;
    case VIEW_ALARM_SET:
      currentState = VIEW_MAIN_MENU;
      alarmSettingField = 0;
      break;
    case VIEW_TIMEZONE_SET:
      currentState = VIEW_MAIN_MENU;
      break;
    case VIEW_DST_SET:
      currentState = VIEW_MAIN_MENU;
      break;
    default:
      break;
  }
}

MenuItem View::getMainViewSelection() const {
  return mainViewSelection;
}

int View::getMainViewCount() const {
  return MAIN_VIEW_COUNT;
}

void View::setWiFiCount(int count) {
  wifiCount = count;
  if (wifiSelection >= count) {
    wifiSelection = 0;
  }
}

int View::getWiFiCount() const {
  return wifiCount;
}

int View::getWiFiSelection() const {
  return wifiSelection;
}

void View::setWiFiSelection(int index) {
  if (index >= 0 && index < wifiCount) {
    wifiSelection = index;
  }
}

int View::getTimeSettingField() const {
  return timeSettingField;
}

void View::setTimeSettingField(int field) {
  timeSettingField = field % 3;
}

void View::adjustTimeValue(int delta) {
  if (timeSettingField == 0) {
    timeHours = (timeHours + delta + 24) % 24;
  } else {
    timeMinutes = (timeMinutes + delta + 60) % 60;
  }
}

uint8_t View::getTimeHours() const {
  return timeHours;
}

uint8_t View::getTimeMinutes() const {
  return timeMinutes;
}

void View::setTimeHours(uint8_t hours) {
  timeHours = hours % 24;
}

void View::setTimeMinutes(uint8_t minutes) {
  timeMinutes = minutes % 60;
}

int View::getAlarmSettingField() const {
  return alarmSettingField;
}

void View::setAlarmSettingField(int field) {
  alarmSettingField = field % 5;
}

void View::adjustAlarmValue(int delta) {
  if (alarmSettingField == 0) {
    alarmHours = (alarmHours + delta + 24) % 24;
  } else if (alarmSettingField == 1) {
    alarmMinutes = (alarmMinutes + delta + 60) % 60;
  } else if (alarmSettingField == 2) {
    alarmEnabled = !alarmEnabled;
  } else if (alarmSettingField == 3) {
    int newLen = (int)alarmLengthSeconds + delta;
    if (newLen < 1) newLen = 1;
    if (newLen > 600) newLen = 600;
    alarmLengthSeconds = (uint16_t)newLen;
  } else if (alarmSettingField == 4) {
    int newVol = (int)alarmVolume + delta;
    if (newVol < 0) newVol = 0;
    if (newVol > 255) newVol = 255;
    alarmVolume = (uint8_t)newVol;
  }
}

uint8_t View::getAlarmHours() const {
  return alarmHours;
}

uint8_t View::getAlarmMinutes() const {
  return alarmMinutes;
}

bool View::getAlarmEnabled() const {
  return alarmEnabled;
}

uint16_t View::getAlarmLengthSeconds() const {
  return alarmLengthSeconds;
}

uint8_t View::getAlarmVolume() const {
  return alarmVolume;
}

void View::setAlarmHours(uint8_t hours) {
  alarmHours = hours % 24;
}

void View::setAlarmMinutes(uint8_t minutes) {
  alarmMinutes = minutes % 60;
}

void View::setAlarmEnabled(bool enabled) {
  alarmEnabled = enabled;
}

void View::setAlarmLengthSeconds(uint16_t seconds) {
  if (seconds < 1) seconds = 1;
  if (seconds > 600) seconds = 600;
  alarmLengthSeconds = seconds;
}

void View::setAlarmVolume(uint8_t volume) {
  alarmVolume = volume;
}

int View::getTimezoneSelection() const {
  return timezoneSelection;
}

void View::setTimezoneSelection(int index) {
  if (index >= 0 && index < 19) {
    timezoneSelection = index;
  }
}

int View::getTimezoneCount() const {
  return 19;
}

bool View::getTimezoneDST() const {
  return timezoneDST;
}

void View::setTimezoneDST(bool enabled) {
  timezoneDST = enabled;
}

void View::toggleTimezoneDST() {
  timezoneDST = !timezoneDST;
}

