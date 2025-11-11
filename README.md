# M5Stack Core 2 Clock

A clock application for the M5Stack Core2, coded in Arduino.

## View Navigation and Control

### Button Controls

- **Button A**: Navigate up / Previous item (or increase brightness in Clock view)
- **Button B**: Navigate down / Next item (or decrease brightness in Clock view)
- **Button C (short press)**: Select / Confirm / Enter view
- **Button C (long press ~500ms)**: Go back / Return to previous view

### Touchscreen Controls

- **Tap menu item**: Select that item
- **Tap Back button**: Go back to previous view (Back button appears in top-left corner of most views)
- **Tap clock view**: Open main menu
- **On-screen keyboard**: Used for WiFi password entry (QWERTY layout with touchable keys)

### View Navigation Flow

#### 1. CLOCK VIEW (default)

- Shows time in HH:mm:ss format
- Tap screen or press Button C → Main Menu
- Button A: Increase brightness
- Button B: Decrease brightness

#### 2. MAIN MENU VIEW

- Shows five options: Set Alarm, WiFi Setup, Set Time, Set Timezone, Set DST
- Button A/B or tap item: Navigate between options (highlighted in green)
- Button C or tap item: Enter selected option
- Tap Back button or Button C (long press): Go back to Clock view

#### 3. WIFI SETUP FLOW

**a) WiFi Scan View**

- Automatically scans for available networks
- Button C or tap: Shows WiFi selection list

**b) WiFi Select View**

- Button A/B or tap item: Navigate through list of WiFi networks
- Button C or tap item: Select network and go to password entry
- Tap Back button or Button C (long press): Go back to Main Menu

**c) WiFi Password View**

- On-screen keyboard: Tap keys to type password (characters shown as \*)
- Button A/B: Ignored (no navigation)
- Tap Enter key or Button C: Confirm password and connect
- Tap Del key: Delete last character
- Tap Back button or Button C (long press): Cancel and go back to WiFi Select view

#### 4. TIME SETTING VIEW

- Shows current time being set (HH:mm) with option to sync from NTP
- Button A/B: Adjust highlighted field (hours, minutes, or NTP sync option)
- Button C: Switch between fields (hours → minutes → NTP sync), or trigger NTP sync
- Button C (long press): Cancel and go back to Main Menu (does not automatically return)

#### 5. ALARM SETTING VIEW

- Shows alarm time, enabled/disabled status, auto-off length, and volume
- Button A/B: Adjust highlighted field (hours, minutes, ON/OFF, length, volume)
- Button C: Switch between fields, or confirm when done
- After cycling through all fields, automatically returns to Clock view
- Button C (long press): Cancel and go back to Main Menu

#### 6. TIMEZONE SETTING VIEW

- Button A/B or tap item: Navigate through timezone list
- Tap item: Select timezone (confirmed when going back)
- Button C (long press) or tap Back: Apply timezone selection and go back to Main Menu

#### 7. DST SETTING VIEW

- Button A/B: Toggle DST ON/OFF
- Button C: Confirm DST setting and return to Clock view
- Button C (long press): Cancel and go back to Main Menu

## Power Management

- **When plugged in**: Full brightness, always on
- **When unplugged**: Reduced brightness, sleep after 30 seconds of inactivity
- Press any button or tap screen to wake from sleep

## NTP Time Sync

- Automatically syncs time from NTP server every 3 minutes when WiFi is connected
- Manual sync available in Time Setting view (Enter on "Sync from NTP now" option)
- Time sync also occurs after WiFi connection and timezone/DST changes
