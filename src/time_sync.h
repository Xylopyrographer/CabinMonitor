#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <Arduino.h>
#include <time.h>

// Sync time with NTP server
bool syncTimeWithNTP();

// Get current time as a formatted string
bool getTimestampString(char* buffer, size_t bufferSize);

// Check if a daily event should occur
bool checkDailyEvent(int hour, int minute);

// Check if a weekly event should occur
bool checkWeeklyEvent(int dayOfWeek, int hour, int minute);

#endif // TIME_SYNC_H
