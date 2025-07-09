#ifndef BUTTON_CONTROL_H
#define BUTTON_CONTROL_H

#include <Arduino.h>

// Button actions
enum ButtonAction {
  BUTTON_NO_ACTION,
  BUTTON_NORMAL_START,
  BUTTON_PROVISIONING,
  BUTTON_OTA_UPDATE,
  BUTTON_FACTORY_RESET,
  BUTTON_DISABLE_MONITORING,
  BUTTON_RESUME_MONITORING
};

// Initialize button control
void initButton();

// Check button state at startup
ButtonAction checkStartupButtonPress();

// Handle button in main loop
ButtonAction handleButton();

// Get monitoring enabled status
bool isMonitoringEnabled();

// Set monitoring enabled status
void setMonitoringEnabled(bool enabled);

// Get time when monitoring was disabled
unsigned long getMonitoringDisabledTime();

// Set time when monitoring was disabled
void setMonitoringDisabledTime(unsigned long time);

#endif // BUTTON_CONTROL_H
