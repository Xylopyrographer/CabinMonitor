#ifndef SMS_MESSAGING_H
#define SMS_MESSAGING_H

#include <Arduino.h>

// Initialize SMS messaging
bool initSMSMessaging();

// Save SMS settings to preferences
bool saveSMSSettings(
  const String& phone,
  const String& activityMsg,
  const String& noActivityMsg,
  const String& noCommMsg,
  const String& commOkMsg,
  const String& disableMsg,
  const String& enableMsg
);

// Send SMS for activity detected
bool sendActivityDetectedSMS();

// Send SMS for no activity detected
bool sendNoActivityDetectedSMS();

// Send SMS for no communication to Google Drive
bool sendNoCommunicationSMS();

// Send SMS for communication to Google Drive OK
bool sendCommunicationOkSMS();

// Send SMS for monitoring disabled
bool sendMonitoringDisabledSMS();

// Send SMS for monitoring enabled
bool sendMonitoringEnabledSMS();

// Generic SMS sender
bool sendSMS(const String& message);

// Get phone number
String getPhoneNumber();

// Get activity detected message
String getActivityDetectedMessage();

// Get no activity detected message
String getNoActivityDetectedMessage();

// Get no communication message
String getNoCommunicationMessage();

// Get communication OK message
String getCommunicationOkMessage();

// Get monitoring disabled message
String getMonitoringDisabledMessage();

// Get monitoring enabled message
String getMonitoringEnabledMessage();

#endif // SMS_MESSAGING_H
