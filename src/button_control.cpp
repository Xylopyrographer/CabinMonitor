#include "button_control.h"
#include "hw_config.h"
#include "config.h"
#include "led_control.h"
#include <XP_Button.h>

// Button instance
XP_Button button(BOOT_MODE_PIN);

// Button state tracking
unsigned long buttonPressStartTime = 0;
unsigned long lastTripleClickTime = 0;
int clickCounter = 0;
bool monitoringEnabled = true;
unsigned long monitoringDisabledTime = 0;

// Initialize button control
void initButton() {
  // Initialize the button with INPUT_PULLUP mode
  button.begin();
  button.setPressTicks(1000);         // Set long press time to 1 second
  button.setLongPressTicks(2000);     // Set very long press time to 2 seconds
  button.setVeryLongPressTicks(10000); // Set super long press time to 10 seconds
}

// Check button state at startup
ButtonAction checkStartupButtonPress() {
  // Wait a bit to allow time for button press detection
  unsigned long startTime = millis();
  
  while (millis() - startTime < 200) {
    button.tick();
    delay(10);
  }
  
  startTime = millis();
  bool buttonPressed = false;
  
  if (button.isPressed()) {
    buttonPressed = true;
    Serial.println("Button is pressed at startup");
    
    // Set LED to indicate button is being held
    setLEDState(LED_BUTTON_PRESSED);
    
    // Wait for button release or timeout
    while (button.isPressed() && (millis() - startTime < 15000)) {
      button.tick();
      delay(10);
      
      // Check for different press durations
      unsigned long pressDuration = millis() - startTime;
      
      if (pressDuration > 10000) {
        setLEDState(LED_FACTORY_RESET);
      } else if (pressDuration > 2000) {
        setLEDState(LED_OTA_MODE);
      } else if (pressDuration > 1000) {
        setLEDState(LED_PROVISIONING);
      }
    }
    
    // Determine action based on press duration
    unsigned long pressDuration = millis() - startTime;
    
    if (pressDuration > 10000) {
      return BUTTON_FACTORY_RESET;
    } else if (pressDuration > 2000) {
      return BUTTON_OTA_UPDATE;
    } else if (pressDuration > 1000) {
      return BUTTON_PROVISIONING;
    }
  }
  
  // No special action
  return BUTTON_NORMAL_START;
}

// Handle button in main loop
ButtonAction handleButton() {
  // Update button state
  button.tick();
  
  // Check for triple click pattern (3 clicks within 3 seconds)
  if (button.wasClicked()) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastTripleClickTime < 3000) {
      // Within 3 second window
      clickCounter++;
      
      if (clickCounter >= 3) {
        // Triple click detected
        clickCounter = 0;
        lastTripleClickTime = 0;
        
        // Toggle monitoring state
        monitoringEnabled = !monitoringEnabled;
        
        if (monitoringEnabled) {
          // Start 20 minute wait period
          monitoringDisabledTime = millis();
          return BUTTON_RESUME_MONITORING;
        } else {
          return BUTTON_DISABLE_MONITORING;
        }
      }
    } else {
      // Start new click sequence
      clickCounter = 1;
      lastTripleClickTime = currentTime;
    }
  }
  
  // Check if we should resume monitoring after wait period
  if (!monitoringEnabled && (millis() - monitoringDisabledTime > MONITORING_RESUME_DELAY_MS)) {
    monitoringEnabled = true;
    return BUTTON_RESUME_MONITORING;
  }
  
  return BUTTON_NO_ACTION;
}

bool isMonitoringEnabled() {
  return monitoringEnabled;
}

void setMonitoringEnabled(bool enabled) {
  monitoringEnabled = enabled;
}

unsigned long getMonitoringDisabledTime() {
  return monitoringDisabledTime;
}

void setMonitoringDisabledTime(unsigned long time) {
  monitoringDisabledTime = time;
}
