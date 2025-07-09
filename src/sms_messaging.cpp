#include "sms_messaging.h"
#include "cellular.h"
#include "config.h"
#include <Preferences.h>

// Default SMS messages
String phoneNumber = "";
String activityDetectedMsg = "Activity detected";
String noActivityDetectedMsg = "No activity detected";
String noCommunicationMsg = "No communication to Google Drive";
String communicationOkMsg = "Communication to Google Drive OK";
String monitoringDisabledMsg = "Monitoring disabled";
String monitoringEnabledMsg = "Monitoring enabled";

// Initialize SMS messaging
bool initSMSMessaging() {
  // Load settings from preferences
  Preferences preferences;
  if (preferences.begin("sms", false)) {
    phoneNumber = preferences.getString("phone", "");
    activityDetectedMsg = preferences.getString("act_msg", activityDetectedMsg);
    noActivityDetectedMsg = preferences.getString("noact_msg", noActivityDetectedMsg);
    noCommunicationMsg = preferences.getString("nocomm_msg", noCommunicationMsg);
    communicationOkMsg = preferences.getString("commok_msg", communicationOkMsg);
    monitoringDisabledMsg = preferences.getString("disabl_msg", monitoringDisabledMsg);
    monitoringEnabledMsg = preferences.getString("enabl_msg", monitoringEnabledMsg);
    
    preferences.end();
    return phoneNumber.length() > 0;
  }
  
  return false;
}

// Save SMS settings to preferences
bool saveSMSSettings(
  const String& phone,
  const String& activityMsg,
  const String& noActivityMsg,
  const String& noCommMsg,
  const String& commOkMsg,
  const String& disableMsg,
  const String& enableMsg
) {
  // Validate phone number (max 13 digits)
  if (phone.length() == 0 || phone.length() > 13) {
    Serial.println("Invalid phone number length");
    return false;
  }
  
  // Check if phone contains only numbers
  for (int i = 0; i < phone.length(); i++) {
    if (!isdigit(phone[i]) && phone[i] != '+') {
      Serial.println("Phone number contains invalid characters");
      return false;
    }
  }
  
  // Validate message lengths (3-80 chars)
  if (activityMsg.length() < 3 || activityMsg.length() > 80 ||
      noActivityMsg.length() < 3 || noActivityMsg.length() > 80 ||
      noCommMsg.length() < 3 || noCommMsg.length() > 80 ||
      commOkMsg.length() < 3 || commOkMsg.length() > 80 ||
      disableMsg.length() < 3 || disableMsg.length() > 80 ||
      enableMsg.length() < 3 || enableMsg.length() > 80) {
    Serial.println("One or more messages have invalid length");
    return false;
  }
  
  // Save to preferences
  Preferences preferences;
  if (preferences.begin("sms", false)) {
    preferences.putString("phone", phone);
    preferences.putString("act_msg", activityMsg);
    preferences.putString("noact_msg", noActivityMsg);
    preferences.putString("nocomm_msg", noCommMsg);
    preferences.putString("commok_msg", commOkMsg);
    preferences.putString("disabl_msg", disableMsg);
    preferences.putString("enabl_msg", enableMsg);
    
    // Update current values
    phoneNumber = phone;
    activityDetectedMsg = activityMsg;
    noActivityDetectedMsg = noActivityMsg;
    noCommunicationMsg = noCommMsg;
    communicationOkMsg = commOkMsg;
    monitoringDisabledMsg = disableMsg;
    monitoringEnabledMsg = enableMsg;
    
    preferences.end();
    return true;
  }
  
  return false;
}

// Send SMS for activity detected
bool sendActivityDetectedSMS() {
  return sendSMS(activityDetectedMsg);
}

// Send SMS for no activity detected
bool sendNoActivityDetectedSMS() {
  return sendSMS(noActivityDetectedMsg);
}

// Send SMS for no communication to Google Drive
bool sendNoCommunicationSMS() {
  return sendSMS(noCommunicationMsg);
}

// Send SMS for communication to Google Drive OK
bool sendCommunicationOkSMS() {
  return sendSMS(communicationOkMsg);
}

// Send SMS for monitoring disabled
bool sendMonitoringDisabledSMS() {
  return sendSMS(monitoringDisabledMsg);
}

// Send SMS for monitoring enabled
bool sendMonitoringEnabledSMS() {
  return sendSMS(monitoringEnabledMsg);
}

// Generic SMS sender
bool sendSMS(const String& message) {
  if (phoneNumber.length() == 0) {
    Serial.println("No phone number configured for SMS");
    return false;
  }
  
  if (!isCellularConnected() && !connectCellular()) {
    Serial.println("Failed to connect cellular for SMS");
    return false;
  }
  
  // Send SMS using AT commands
  String response;
  
  // Set SMS to text mode
  response = sendATCommand("AT+CMGF=1");
  if (response.indexOf("OK") == -1) {
    Serial.println("Failed to set SMS text mode");
    return false;
  }
  
  // Set phone number
  String cmd = "AT+CMGS=\"" + phoneNumber + "\"";
  response = sendATCommand(cmd, 5000);
  
  if (response.indexOf(">") == -1) {
    Serial.println("Failed to set SMS recipient");
    return false;
  }
  
  // Send the message content followed by Ctrl+Z (ASCII 26)
  String msgWithCtrlZ = message + char(26);
  response = sendATCommand(msgWithCtrlZ, 10000);
  
  if (response.indexOf("+CMGS:") == -1) {
    Serial.println("Failed to send SMS");
    return false;
  }
  
  Serial.println("SMS sent successfully");
  return true;
}

// Get phone number
String getPhoneNumber() {
  return phoneNumber;
}

// Get activity detected message
String getActivityDetectedMessage() {
  return activityDetectedMsg;
}

// Get no activity detected message
String getNoActivityDetectedMessage() {
  return noActivityDetectedMsg;
}

// Get no communication message
String getNoCommunicationMessage() {
  return noCommunicationMsg;
}

// Get communication OK message
String getCommunicationOkMessage() {
  return communicationOkMsg;
}

// Get monitoring disabled message
String getMonitoringDisabledMessage() {
  return monitoringDisabledMsg;
}

// Get monitoring enabled message
String getMonitoringEnabledMessage() {
  return monitoringEnabledMsg;
}
