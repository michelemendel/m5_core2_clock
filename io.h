#ifndef IO_H
#define IO_H

#include <Arduino.h>

enum InputEvent {
  INPUT_NONE,
  INPUT_BUTTON_UP,
  INPUT_BUTTON_DOWN,
  INPUT_BUTTON_SELECT,
  INPUT_BUTTON_BACK,
  INPUT_KEY_PRESSED,
  INPUT_TOUCH_SELECT,
  INPUT_TOUCH_BACK
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
  
  // Touchscreen coordinate getters
  int getTouchX() const { return lastTouchX; }
  int getTouchY() const { return lastTouchY; }
  
  bool isCharging() const;
  void handlePowerManagement();
  
private:
  InputEvent lastEvent;
  char lastKey;
  bool lastButtonState[3]; // A, B, C buttons
  unsigned long lastDebounceTime[3]; // Separate debounce timer for each button
  static const unsigned long DEBOUNCE_DELAY = 50;
  static const unsigned long LONG_PRESS_DELAY = 500; // Long press for back button
  bool wasCharging;
  bool textInputMode = false;
  
  // Button press tracking for long press detection
  unsigned long buttonCPressStart = 0;
  bool buttonCWasPressed = false;
  
  // Touchscreen state
  int lastTouchX = -1;
  int lastTouchY = -1;
  unsigned long lastTouchTime = 0;
  static const unsigned long TOUCH_DEBOUNCE_DELAY = 200; // Prevent double-taps
};

#endif // IO_H

