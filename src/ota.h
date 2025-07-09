#ifndef OTA_H
#define OTA_H

#include <Arduino.h>

// Initialize OTA update functionality
bool initOTA();

// Handle OTA update process (call in loop when in OTA mode)
void handleOTA();

#endif // OTA_H
