#include "io.h"
#include <M5Unified.h>

IO::IO() {
  lastEvent = INPUT_NONE;
  lastKey = 0;
  for (int i = 0; i < 3; i++) {
    lastButtonState[i] = false;
    lastDebounceTime[i] = 0;
  }
  wasCharging = false;
  textInputMode = false;
  buttonCPressStart = 0;
  buttonCWasPressed = false;
  lastTouchX = -1;
  lastTouchY = -1;
  lastTouchTime = 0;
}

void IO::begin() {
  // Core2 buttons and touchscreen are initialized via M5.begin()
  // which should be called in main setup()
}

void IO::setTextInputMode(bool enabled) {
  textInputMode = enabled;
}

void IO::update() {
  unsigned long currentTime = millis();
  
  // Check buttons FIRST before anything else, and set event immediately
  // Handle button A (Up) - use proper edge detection
  bool buttonAPressed = M5.BtnA.isPressed();
  bool buttonAJustPressed = buttonAPressed && !lastButtonState[0];
  
  // Handle button B (Down) - use proper edge detection  
  bool buttonBPressed = M5.BtnB.isPressed();
  bool buttonBJustPressed = buttonBPressed && !lastButtonState[1];
  
  // If either button A or B was just pressed, set the event immediately
  if (buttonAJustPressed && (currentTime - lastDebounceTime[0] > DEBOUNCE_DELAY)) {
    lastEvent = INPUT_BUTTON_UP;
    lastDebounceTime[0] = currentTime;
    lastButtonState[0] = true;
    lastButtonState[1] = buttonBPressed;
    lastButtonState[2] = M5.BtnC.isPressed();
    // Clear ALL other input sources
    lastKey = 0;
    lastTouchX = -1;
    lastTouchY = -1;
    handlePowerManagement();
    return; // Exit immediately - don't process anything else
  }
  
  if (buttonBJustPressed && (currentTime - lastDebounceTime[1] > DEBOUNCE_DELAY)) {
    lastEvent = INPUT_BUTTON_DOWN;
    lastDebounceTime[1] = currentTime;
    lastButtonState[0] = buttonAPressed;
    lastButtonState[1] = true;
    lastButtonState[2] = M5.BtnC.isPressed();
    // Clear ALL other input sources
    lastKey = 0;
    lastTouchX = -1;
    lastTouchY = -1;
    handlePowerManagement();
    return; // Exit immediately - don't process anything else
  }
  
  // Update button states
  lastButtonState[0] = buttonAPressed;
  lastButtonState[1] = buttonBPressed;
  
  // Only reset lastEvent if it was already consumed (INPUT_NONE)
  // This allows the event to persist until getInputEvent() is called
  if (lastEvent == INPUT_NONE) {
    lastKey = 0;
    lastTouchX = -1;
    lastTouchY = -1;
  }
  
  // Handle button C (Select/Back with long press)
  bool buttonCPressed = M5.BtnC.isPressed();
  if (buttonCPressed && !lastButtonState[2]) {
    // Button C just pressed - start timing for long press
    buttonCPressStart = currentTime;
    buttonCWasPressed = true;
  } else if (!buttonCPressed && lastButtonState[2]) {
    // Button C just released
    if (buttonCWasPressed) {
      unsigned long pressDuration = currentTime - buttonCPressStart;
      if (pressDuration < LONG_PRESS_DELAY) {
        // Short press = Select
        if (currentTime - lastDebounceTime[2] > DEBOUNCE_DELAY) {
          lastEvent = INPUT_BUTTON_SELECT;
          lastDebounceTime[2] = currentTime;
          // Clear touch coordinates when button is pressed to prevent accidental touch processing
          lastTouchX = -1;
          lastTouchY = -1;
          handlePowerManagement();
        }
      } else {
        // Long press = Back
        if (currentTime - lastDebounceTime[2] > DEBOUNCE_DELAY) {
          lastEvent = INPUT_BUTTON_BACK;
          lastDebounceTime[2] = currentTime;
          // Clear touch coordinates when button is pressed to prevent accidental touch processing
          lastTouchX = -1;
          lastTouchY = -1;
          handlePowerManagement();
        }
      }
      buttonCWasPressed = false;
    }
  } else if (buttonCPressed && buttonCWasPressed) {
    // Button C still pressed - check if it's been long enough for back
    unsigned long pressDuration = currentTime - buttonCPressStart;
    if (pressDuration >= LONG_PRESS_DELAY && currentTime - lastDebounceTime[2] > DEBOUNCE_DELAY) {
      // Long press detected while still pressed
      lastEvent = INPUT_BUTTON_BACK;
      lastDebounceTime[2] = currentTime;
      // Clear touch coordinates when button is pressed to prevent accidental touch processing
      lastTouchX = -1;
      lastTouchY = -1;
      buttonCWasPressed = false; // Prevent repeat events
      handlePowerManagement();
    }
  }
  lastButtonState[2] = buttonCPressed;
  
  // Handle touchscreen input - but only if no buttons are currently pressed
  // This prevents accidental touch processing when buttons are being used
  bool anyButtonPressed = M5.BtnA.isPressed() || M5.BtnB.isPressed() || M5.BtnC.isPressed();
  if (!anyButtonPressed) {
    auto touch = M5.Touch.getDetail();
    if (touch.wasPressed() && (currentTime - lastTouchTime > TOUCH_DEBOUNCE_DELAY)) {
      lastTouchX = touch.x;
      lastTouchY = touch.y;
      lastTouchTime = currentTime;
      
      // Touch events will be processed by display module to determine what was touched
      // For now, we'll just store coordinates and let display module handle interpretation
      // The display module will call back to set specific events if needed
    }
  } else {
    // Clear touch coordinates when any button is pressed
    lastTouchX = -1;
    lastTouchY = -1;
  }
  
  // Handle power management
  handlePowerManagement();
}

InputEvent IO::getInputEvent() {
  return lastEvent;
}

char IO::getLastKey() {
  return lastKey;
}

void IO::clearInput() {
  lastEvent = INPUT_NONE;
  lastKey = 0;
  lastTouchX = -1;
  lastTouchY = -1;
}

bool IO::isCharging() const {
  return M5.Power.isCharging();
}

void IO::handlePowerManagement() {
  bool charging = isCharging();
  
  if (!charging && wasCharging) {
    // Just unplugged - prepare for sleep
    // Sleep will be handled in main loop
  }
  
  wasCharging = charging;
}
