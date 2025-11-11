#include "io.h"
#include <M5Unified.h>
#include <M5Cardputer.h>

IO::IO() {
  lastEvent = INPUT_NONE;
  lastKey = 0;
  for (int i = 0; i < 3; i++) {
    lastButtonState[i] = false;
  }
  lastDebounceTime = 0;
  wasCharging = false;
}

void IO::begin() {
  // Cardputer keyboard is initialized via M5Cardputer.begin()
  // which should be called in main setup()
}

void IO::setTextInputMode(bool enabled) {
  textInputMode = enabled;
}

void IO::update() {
  lastEvent = INPUT_NONE;
  lastKey = 0;
  
  unsigned long currentTime = millis();
  // Reset partial ESC state if idle (longer timeout to catch split sequences)
  if (escState > 0 && (currentTime - escStartTime) > 1000) {
    escState = 0;
  }
  
  // Update M5Cardputer which also updates keyboard (matches example exactly)
  M5Cardputer.update();
  
  // Check for keyboard input changes (pattern matches M5Cardputer example exactly)
  if (M5Cardputer.Keyboard.isChange()) {
    if (M5Cardputer.Keyboard.isPressed()) {
      Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
      
      // Apply debounce to prevent multiple rapid events
      if (currentTime - lastDebounceTime > DEBOUNCE_DELAY) {
        // Process Enter key first
        if (status.enter) {
          lastEvent = INPUT_BUTTON_SELECT;
          lastDebounceTime = currentTime;
          handlePowerManagement();
          return;
        }
        
        // Process Delete key
        if (status.del) {
          lastKey = '\b';
          lastEvent = INPUT_KEY_PRESSED;
          lastDebounceTime = currentTime;
          handlePowerManagement();
          return;
        }
        
        // Parse ESC sequence across characters: ESC [ A/B (only when not in text mode)
        if (!textInputMode && status.word.size() > 0) {
          for (auto c : status.word) {
            if (escState == 0) {
              if (c == 27) { escState = 1; escStartTime = currentTime; continue; }
            } else if (escState == 1) { // after ESC
              if (c == '[') { escState = 2; escStartTime = currentTime; continue; }
              // Not an arrow sequence, treat as ESC key
              lastKey = 27;
              lastEvent = INPUT_KEY_PRESSED;
              escState = 0;
              lastDebounceTime = currentTime;
              handlePowerManagement();
              return;
            } else if (escState == 2) { // after ESC[
              if (c == 'A') { // Up
                lastEvent = INPUT_BUTTON_UP;
                escState = 0;
                lastDebounceTime = currentTime;
                handlePowerManagement();
                return;
              } else if (c == 'B') { // Down
                lastEvent = INPUT_BUTTON_DOWN;
                escState = 0;
                lastDebounceTime = currentTime;
                handlePowerManagement();
                return;
              } else {
                // Unknown ESC sequence -> reset
                escState = 0;
              }
            }
          }
        }
        
        // Process ALL characters in word (matching example pattern)
        if (status.word.size() > 0) {
          // Process each character (could be multiple keys pressed)
          for (auto c : status.word) {
            if (textInputMode) {
              // In text mode: treat all printable chars as text; ignore ESC sequences here
              if (c >= 32 && c <= 126) {
                lastKey = c;
                lastEvent = INPUT_KEY_PRESSED;
                lastDebounceTime = currentTime;
                handlePowerManagement();
                return;
              } else if (c == 27) {
                // ignore ESC here; no back via ESC
                continue;
              }
            } else {
              // Navigation and back key mapping (check BEFORE setting as regular key)
              if (c == 'w' || c == 'W' || c == ';') { // Up: W or ';'
                lastEvent = INPUT_BUTTON_UP;
                lastDebounceTime = currentTime;
                handlePowerManagement();
                return;  // Exit immediately on navigation key
              } else if (c == '.' || c == 'a' || c == 'A') { // Down: '.' or 'a'
                lastEvent = INPUT_BUTTON_DOWN;
                lastDebounceTime = currentTime;
                handlePowerManagement();
                return;  // Exit immediately on navigation key
              } else if (c == 'q' || c == 'Q') { // Back: Q
                lastKey = 'q';
                lastEvent = INPUT_KEY_PRESSED;
                lastDebounceTime = currentTime;
                handlePowerManagement();
                return;  // Exit immediately on back key
              } else if (c == 27) {
                // ESC handled by state machine; defer until sequence resolved
                continue;
              } else {
                // Regular text input - process first non-navigation character
                lastKey = c;
                lastEvent = INPUT_KEY_PRESSED;
                lastDebounceTime = currentTime;
                handlePowerManagement();
                return;  // Process one character at a time
              }
            }
          }
        }
      }
    }
  }
  
  // If ESC was pressed alone and we didn't get '[' within a short time, ignore (no ESC back)
  if (!textInputMode && lastEvent == INPUT_NONE && escState == 1 && (millis() - escStartTime) > 150) {
    escState = 0;
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

