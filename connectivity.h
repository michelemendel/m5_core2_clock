#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#define NTP_SYNC_INTERVAL_MINUTES 30

class Connectivity {
public:
  Connectivity();
  
  void begin();
  void update();
  
  bool scanWiFi(String* ssids, int maxCount, int& count);
  // Blocking connect (legacy). Prefer beginConnect + update for non-blocking
  bool connectWiFi(const String& ssid, const String& password);
  // Non-blocking connect API
  void beginConnect(const String& ssid, const String& password, unsigned long timeoutMs = 15000);
  bool isConnecting() const;
  bool connectFinished() const; // true when either success or failure reached
  bool connectSucceeded() const;
  void disconnectWiFi();
  bool isConnected() const;
  String getSSID() const;
  
  bool syncTime();
  bool syncTime(int8_t timezoneOffset, bool daylightSaving);
  bool isTimeSynced() const;
  unsigned long getLastSyncTime() const;
  bool shouldSync() const;
  
  time_t getNTPTime();
  
private:
  bool wifiConnected;
  String currentSSID;
  String currentPassword;  // Store password for auto-reconnection
  bool timeSynced;
  unsigned long lastSyncTime;
  // Async connect state
  bool connecting = false;
  bool connectDone = false;
  bool connectOk = false;
  unsigned long connectStartMs = 0;
  unsigned long connectTimeoutMs = 15000;
  String pendingSSID = "";
  String pendingPWD = "";
  // Auto-reconnection state
  unsigned long lastReconnectAttemptMs = 0;
  static const unsigned long RECONNECT_INTERVAL_MS = 30000;  // Try to reconnect every 30 seconds
  static const char* NTP_SERVER;
  static const int TIMEZONE_OFFSET_HOURS;  // Default timezone offset in hours (e.g., -5 for EST)
  static const long GMT_OFFSET_SEC;
  static const int DAYLIGHT_OFFSET_SEC;
};

#endif // CONNECTIVITY_H

