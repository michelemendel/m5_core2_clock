#include "connectivity.h"
#include <time.h>

const char* Connectivity::NTP_SERVER = "pool.ntp.org";
// Timezone offset in hours (e.g., -5 for EST, -8 for PST, +1 for CET)
// This will be converted to seconds for configTime()
const int Connectivity::TIMEZONE_OFFSET_HOURS = -5; // Default: EST (Eastern Standard Time)
const long Connectivity::GMT_OFFSET_SEC = Connectivity::TIMEZONE_OFFSET_HOURS * 3600L;
const int Connectivity::DAYLIGHT_OFFSET_SEC = 3600; // 1 hour for daylight saving (adjust if your region uses DST)

Connectivity::Connectivity() {
  wifiConnected = false;
  currentSSID = "";
  currentPassword = "";
  timeSynced = false;
  lastSyncTime = 0;
  lastReconnectAttemptMs = 0;
}

void Connectivity::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  // Configure WiFi for better reliability
  WiFi.setAutoReconnect(true);  // Enable ESP32's built-in auto-reconnect
  WiFi.setSleep(false);  // Disable WiFi sleep to prevent disconnections
}

void Connectivity::update() {
  // Check WiFi connection status
  bool currentlyConnected = (WiFi.status() == WL_CONNECTED);
  
  if (wifiConnected && !currentlyConnected) {
    // WiFi disconnected - mark as disconnected
    wifiConnected = false;
    timeSynced = false;
    // Don't clear currentSSID/password yet - we'll try to reconnect
    lastReconnectAttemptMs = millis(); // Start reconnection timer
  } else if (!wifiConnected && currentlyConnected) {
    // WiFi reconnected - update state
    wifiConnected = true;
    timeSynced = false; // Will need to resync time
    lastReconnectAttemptMs = 0; // Reset reconnect timer
  }

  // Auto-reconnection logic: if disconnected and we have credentials, try to reconnect
  if (!wifiConnected && !connecting && currentSSID.length() > 0 && currentPassword.length() > 0) {
    unsigned long now = millis();
    // Check if enough time has passed since last reconnect attempt
    if (lastReconnectAttemptMs == 0 || (now - lastReconnectAttemptMs) >= RECONNECT_INTERVAL_MS) {
      // Attempt non-blocking reconnection
      beginConnect(currentSSID, currentPassword, 15000);
      lastReconnectAttemptMs = now;
    }
  }

  // Progress non-blocking connection if active
  if (connecting && !connectDone) {
    // If just started, kick off WiFi begin
    if (connectStartMs == 0) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(pendingSSID.c_str(), pendingPWD.c_str());
      connectStartMs = millis();
    }
    wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED) {
      wifiConnected = true;
      currentSSID = pendingSSID;
      currentPassword = pendingPWD;  // Store password for future reconnection
      connectOk = true;
      connectDone = true;
      connecting = false;
      lastReconnectAttemptMs = 0; // Reset reconnect timer on success
    } else if ((millis() - connectStartMs) >= connectTimeoutMs || status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
      // Timeout or immediate error => done with failure
      connectOk = false;
      connectDone = true;
      connecting = false;
      WiFi.disconnect(true);
      // Update reconnect timer to current time so we wait for interval before retry
      lastReconnectAttemptMs = millis();
    }
  }
}

bool Connectivity::scanWiFi(String* ssids, int maxCount, int& count) {
  count = 0;
  
  // Disconnect any existing connection before scanning
  WiFi.disconnect();
  delay(200);
  
  // Wait for disconnect to complete (status becomes WL_DISCONNECTED or WL_IDLE_STATUS)
  int waitCount = 0;
  while (WiFi.status() != WL_DISCONNECTED && waitCount < 10) {
    delay(50);
    waitCount++;
  }
  
  WiFi.mode(WIFI_STA);
  delay(100);
  
  // Start scan
  int n = WiFi.scanNetworks();
  
  if (n == -1) {
    return false;
  }
  
  count = min(n, maxCount);
  for (int i = 0; i < count; i++) {
    ssids[i] = WiFi.SSID(i);
  }
  
  return true;
}

bool Connectivity::connectWiFi(const String& ssid, const String& password) {
  // Completely stop WiFi first
  WiFi.mode(WIFI_OFF);
  delay(100);
  
  // Re-enable in station mode
  WiFi.mode(WIFI_STA);
  delay(200);
  
  // Force disconnect and clear any saved config
  WiFi.disconnect(true);  // true = delete old config
  delay(500);
  
  // Wait until fully disconnected - check multiple times
  int waitCount = 0;
  while (waitCount < 30) {  // Max 3 seconds wait
    wl_status_t status = WiFi.status();
    if (status == WL_DISCONNECTED || status == WL_IDLE_STATUS) {
      break;  // Successfully disconnected
    }
    // Still connected or connecting - force disconnect again
    WiFi.disconnect(true);
    delay(100);
    waitCount++;
  }
  
  // Additional delay to ensure WiFi stack is ready
  delay(300);
  
  // Now start connection - this should be the only time WiFi.begin is called
  WiFi.begin(ssid.c_str(), password.c_str());
  
  // Wait for connection with timeout (20 seconds total)
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    attempts++;
    
    // Check for connection errors
    wl_status_t status = WiFi.status();
    if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
      // Connection failed
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      delay(100);
      WiFi.mode(WIFI_STA);
      return false;
    }
    
    // Prevent infinite loop if stuck
    if (attempts == 20) {
      // Halfway through - check if we're making progress
      // If status hasn't changed, might need to restart
      static wl_status_t lastStatus = WL_DISCONNECTED;
      if (status == lastStatus && status != WL_DISCONNECTED) {
        // Stuck in same state - try once more
        WiFi.disconnect(true);
        delay(200);
        WiFi.begin(ssid.c_str(), password.c_str());
      }
      lastStatus = status;
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    currentSSID = ssid;
    currentPassword = password;  // Store password for auto-reconnection
    lastReconnectAttemptMs = 0;  // Reset reconnect timer
    return true;
  }
  
  // Connection failed - clean up completely
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_STA);
  return false;
}

void Connectivity::beginConnect(const String& ssid, const String& password, unsigned long timeoutMs) {
  pendingSSID = ssid;
  pendingPWD = password;
  connectTimeoutMs = timeoutMs;
  connectStartMs = 0; // mark not yet started
  connectOk = false;
  connectDone = false;
  connecting = true;
}

bool Connectivity::isConnecting() const {
  return connecting && !connectDone;
}

bool Connectivity::connectFinished() const {
  return connectDone;
}

bool Connectivity::connectSucceeded() const {
  return connectDone && connectOk;
}

void Connectivity::disconnectWiFi() {
  WiFi.disconnect();
  wifiConnected = false;
  currentSSID = "";
  currentPassword = "";  // Clear password when manually disconnected
  timeSynced = false;
  lastReconnectAttemptMs = 0;  // Reset reconnect timer
}

bool Connectivity::isConnected() const {
  return wifiConnected && WiFi.status() == WL_CONNECTED;
}

String Connectivity::getSSID() const {
  return currentSSID;
}

bool Connectivity::syncTime() {
  return syncTime(TIMEZONE_OFFSET_HOURS, true);
}

bool Connectivity::syncTime(int8_t timezoneOffset, bool daylightSaving) {
  if (!isConnected()) {
    return false;
  }
  
  long gmtOffset = timezoneOffset * 3600L;
  int daylightOffset = daylightSaving ? 3600 : 0;
  
  configTime(gmtOffset, daylightOffset, NTP_SERVER);
  
  // Try to get time with a short, non-blocking polling loop (~2s max)
  struct tm timeinfo;
  const int maxAttempts = 20; // 20 * 100ms = ~2s
  for (int i = 0; i < maxAttempts; ++i) {
    if (getLocalTime(&timeinfo, 0)) {
      timeSynced = true;
      lastSyncTime = millis();
      return true;
    }
    delay(100);
  }

  if (getLocalTime(&timeinfo, 0)) {
    timeSynced = true;
    lastSyncTime = millis();
    return true;
  }
  
  return false;
}

bool Connectivity::isTimeSynced() const {
  return timeSynced;
}

unsigned long Connectivity::getLastSyncTime() const {
  return lastSyncTime;
}

bool Connectivity::shouldSync() const {
  if (!isConnected()) {
    return false;
  }
  if (!timeSynced) {
    return true; // initial sync as soon as connected
  }
  unsigned long elapsed = (millis() - lastSyncTime) / 1000 / 60;
  return elapsed >= NTP_SYNC_INTERVAL_MINUTES;
}

time_t Connectivity::getNTPTime() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    return mktime(&timeinfo);
  }
  return 0;
}

