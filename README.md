# m5_clock

A clock application for the M5Stack Cardputer, coded in Arduino.

## View Navigation and Control

### Button Controls

- **Enter**: Select / Confirm / Enter view
- **q or Q**: Go back / Return to previous view
- **Arrow Up, w, W, or ;**: Navigate up / Previous item (or increase brightness in Clock view)
- **Arrow Down, a, A, or .**: Navigate down / Next item (or decrease brightness in Clock view)
- **Keyboard**: Enter WiFi password (when in password view)
  - All other keys are treated as text input in password view

### View Navigation Flow

#### 1. CLOCK VIEW (default)

- Shows time in HH:mm:ss format
- Press Enter → Main Menu
- Arrow Up: Increase brightness
- Arrow Down: Decrease brightness

#### 2. MAIN MENU VIEW

- Shows five options: Set Alarm, WiFi Setup, Set Time, Set Timezone, Set DST
- Arrow Up/Down: Navigate between options (highlighted in green)
- Enter: Enter selected option
- q or Q: Go back to Clock view

#### 3. WIFI SETUP FLOW

**a) WiFi Scan View**

- Automatically scans for available networks
- Enter: Shows WiFi selection list

**b) WiFi Select View**

- Arrow Up/Down: Navigate through list of WiFi networks
- Enter: Select network and go to password entry
- q or Q: Go back to Main Menu

**c) WiFi Password View**

- Keyboard: Type password (characters shown as \*)
- Arrow Up/Down: Ignored (no navigation)
- Enter: Confirm password and connect
- Backspace: Delete last character
- q or Q: Cancel and go back to WiFi Select view

#### 4. TIME SETTING VIEW

- Shows current time being set (HH:mm) with option to sync from NTP
- Arrow Up/Down: Adjust highlighted field (hours, minutes, or NTP sync option)
- Enter: Switch between fields (hours → minutes → NTP sync), or trigger NTP sync
- q or Q: Cancel and go back to Main Menu (does not automatically return)

#### 5. ALARM SETTING VIEW

- Shows alarm time, enabled/disabled status, auto-off length, and volume
- Arrow Up/Down: Adjust highlighted field (hours, minutes, ON/OFF, length, volume)
- Enter: Switch between fields, or confirm when done
- After cycling through all fields, automatically returns to Clock view
- q or Q: Cancel and go back to Main Menu

#### 6. TIMEZONE SETTING VIEW

- Arrow Up/Down: Navigate through timezone list
- Enter: No action (timezone is confirmed when going back)
- q or Q: Apply timezone selection and go back to Main Menu

#### 7. DST SETTING VIEW

- Arrow Up/Down: Toggle DST ON/OFF
- Enter: Confirm DST setting and return to Clock view
- q or Q: Cancel and go back to Main Menu

## Power Management

- **When plugged in**: Full brightness, always on
- **When unplugged**: Reduced brightness, sleep after 30 seconds of inactivity
- Press any button/key to wake from sleep

## NTP Time Sync

- Automatically syncs time from NTP server every 3 minutes when WiFi is connected
- Manual sync available in Time Setting view (Enter on "Sync from NTP now" option)
- Time sync also occurs after WiFi connection and timezone/DST changes
