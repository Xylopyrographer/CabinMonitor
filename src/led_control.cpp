#include "led_control.h"
#include "hw_config.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel strip(NUM_LEDS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);
LEDState currentLEDState = LED_INIT;
unsigned long lastBlinkTime = 0;
unsigned long lastDoubleBlinkTime = 0;
int blinkPhase = 0;
bool blinkState = false;

void initLED() {
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.setPixelColor(0, 0, 0, 0);
  strip.show(); // Initialize all pixels to 'off'
  currentLEDState = LED_INIT;
}

void setLEDState(LEDState state) {
  currentLEDState = state;
  blinkState = false;  // Reset blink state
  blinkPhase = 0;      // Reset blink phase for double blink
  lastBlinkTime = millis();
  lastDoubleBlinkTime = millis();
  updateLED();
}

void updateLED() {
  uint32_t color;
  bool shouldBlink = false;
  bool shouldDoubleBlink = false;
  
  // Determine color based on state
  switch (currentLEDState) {
    case LED_INIT:
      color = strip.Color(0, 0, 255);  // Blue during initialization
      break;
      
    case LED_IDLE:
      color = LED_COLOR_IDLE;
      break;
      
    case LED_PIR_DETECTED:
      color = LED_COLOR_PIR_DETECTED;
      break;
      
    case LED_SOUND_DETECTED:
      color = LED_COLOR_SOUND_DETECTED;
      break;
      
    case LED_CAPTURING:
      color = LED_COLOR_CAPTURING;
      break;
      
    case LED_UPLOADING:
      color = LED_COLOR_UPLOADING;
      break;
      
    case LED_OTA_MODE:
      color = LED_COLOR_OTA_MODE;
      break;
      
    case LED_ERROR:
      color = LED_COLOR_ERROR;
      shouldBlink = true;
      break;
      
    case LED_PROVISIONING:
      color = LED_COLOR_PROVISIONING;
      shouldBlink = true;
      break;
      
    case LED_BUTTON_PRESSED:
      color = LED_COLOR_BUTTON_PRESSED;
      break;
      
    case LED_FACTORY_RESET:
      color = LED_COLOR_FACTORY_RESET;
      shouldBlink = true;
      break;
      
    case LED_MONITORING_DISABLED:
      color = LED_COLOR_MONITORING_DISABLED;
      shouldDoubleBlink = true;
      break;
      
    default:
      color = strip.Color(0, 0, 0);  // Off
      break;
  }
  
  // Handle blinking for states that need it
  if (shouldBlink) {
    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime >= LED_BLINK_INTERVAL_MS) {
      blinkState = !blinkState;
      lastBlinkTime = currentTime;
    }
    
    if (!blinkState) {
      color = strip.Color(0, 0, 0);  // Off during blink cycle
    }
  }
  // Handle double blinking (quick double flash followed by longer pause)
  else if (shouldDoubleBlink) {
    unsigned long currentTime = millis();
    
    // State machine for double blink pattern
    switch (blinkPhase) {
      case 0: // First ON phase
        if (currentTime - lastDoubleBlinkTime >= 200) {
          blinkPhase = 1;
          lastDoubleBlinkTime = currentTime;
        }
        break;
        
      case 1: // First OFF phase
        color = strip.Color(0, 0, 0);
        if (currentTime - lastDoubleBlinkTime >= 200) {
          blinkPhase = 2;
          lastDoubleBlinkTime = currentTime;
        }
        break;
        
      case 2: // Second ON phase
        if (currentTime - lastDoubleBlinkTime >= 200) {
          blinkPhase = 3;
          lastDoubleBlinkTime = currentTime;
        }
        break;
        
      case 3: // Long OFF phase
        color = strip.Color(0, 0, 0);
        if (currentTime - lastDoubleBlinkTime >= 60000 - 600) { // One minute minus the time used for the flashes
          blinkPhase = 0;
          lastDoubleBlinkTime = currentTime;
        }
        break;
    }
  }
  
  // Set the LED color
  strip.setPixelColor(0, color);
  strip.show();
}
