#include "time_sync.h"
#include "cellular.h"
#include "config.h"

// Sync time with NTP server
bool syncTimeWithNTP() {
  if (!isCellularConnected() && !connectCellular()) {
    Serial.println("Failed to connect cellular for NTP sync");
    return false;
  }
  
  // Use the SIM7000G module to sync time with NTP
  String response;
  
  // Set timezone
  response = sendATCommand("AT+CNTP=\"" + String(NTP_SERVER) + "\"," + String(NTP_TIMEZONE_OFFSET_SEC / 3600));
  if (response.indexOf("OK") == -1) {
    Serial.println("Failed to set NTP server");
    return false;
  }
  
  // Request time sync
  response = sendATCommand("AT+CNTP", 10000);
  if (response.indexOf("+CNTP: 1") == -1) {
    Serial.println("NTP request failed");
    return false;
  }
  
  // Get network time
  response = sendATCommand("AT+CCLK?");
  if (response.indexOf("+CCLK:") == -1) {
    Serial.println("Failed to get network time");
    return false;
  }
  
  // Parse time response (format: +CCLK: "yy/MM/dd,hh:mm:ss±zz")
  int timeIndex = response.indexOf("\"");
  if (timeIndex == -1) {
    Serial.println("Invalid time format");
    return false;
  }
  
  String timeStr = response.substring(timeIndex + 1, response.lastIndexOf("\""));
  Serial.println("Network time: " + timeStr);
  
  // Parse date/time components
  int year = 2000 + timeStr.substring(0, 2).toInt();
  int month = timeStr.substring(3, 5).toInt();
  int day = timeStr.substring(6, 8).toInt();
  int hour = timeStr.substring(9, 11).toInt();
  int minute = timeStr.substring(12, 14).toInt();
  int second = timeStr.substring(15, 17).toInt();
  
  // Set ESP32 system time
  struct tm timeinfo;
  timeinfo.tm_year = year - 1900;
  timeinfo.tm_mon = month - 1;
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = hour;
  timeinfo.tm_min = minute;
  timeinfo.tm_sec = second;
  
  time_t t = mktime(&timeinfo);
  struct timeval now = { .tv_sec = t, .tv_usec = 0 };
  settimeofday(&now, NULL);
  
  Serial.printf("Time set to: %04d-%02d-%02d %02d:%02d:%02d\n", 
                year, month, day, hour, minute, second);
  
  return true;
}

// Get current time as a formatted string
bool getTimestampString(char* buffer, size_t bufferSize) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    // If unable to get time, use a placeholder
    strncpy(buffer, "yyyyMMdd_HHMMSS", bufferSize);
    return false;
  }
  
  strftime(buffer, bufferSize, TIME_FORMAT, &timeinfo);
  return true;
}

// Check if a daily event should occur
bool checkDailyEvent(int hour, int minute) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return false;
  }
  
  return (timeinfo.tm_hour == hour && timeinfo.tm_min == minute);
}

// Check if a weekly event should occur
bool checkWeeklyEvent(int dayOfWeek, int hour, int minute) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return false;
  }
  
  return (timeinfo.tm_wday == dayOfWeek && 
          timeinfo.tm_hour == hour && 
          timeinfo.tm_min == minute);
}
