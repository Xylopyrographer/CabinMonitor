#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>
#include "hw_config.h"

// Initialize the WS2812 RGB LED
void initLED();

// Set the LED to a specific state
void setLEDState(LEDState state);

// Updates LED status (call in loop for blinking effects)
void updateLED();

#endif // LED_CONTROL_H
