#ifndef IO_H
#define IO_H

#include <Arduino.h>

enum InputEvent {
  INPUT_NONE,
  INPUT_BUTTON_UP,
  INPUT_BUTTON_DOWN,
  INPUT_BUTTON_SELECT,
  INPUT_KEY_PRESSED
};

class IO {
public:
  IO();
  
  void begin();
  void update();
  
  InputEvent getInputEvent();
  char getLastKey();
  void clearInput();
  void setTextInputMode(bool enabled);
  bool isTextInputMode() const { return textInputMode; }
  
  bool isCharging() const;
  void handlePowerManagement();
  
private:
  InputEvent lastEvent;
  char lastKey;
  bool lastButtonState[3]; // Up, Down, Select
  unsigned long lastDebounceTime;
  static const unsigned long DEBOUNCE_DELAY = 30;  // Reduced from 50ms to be more responsive
  bool wasCharging;
  // Simple ESC/Arrow state machine: 0=none,1=ESC,2=ESC[
  uint8_t escState = 0;
  unsigned long escStartTime = 0;
  bool textInputMode = false;
};

#endif // IO_H

