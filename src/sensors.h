#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "driver/i2s.h"

// Initialization functions
bool initSensors();

// PIR sensor functions
bool isPIRTriggered();

// MEMS microphone functions
bool isSoundDetected();
int16_t getSoundLevel();

// Light sensor functions
int getLightLevel();

// IR LED and IR cut control
void enableIRLEDs(bool enable);
void enableIRCut(bool enable);

#endif // SENSORS_H
