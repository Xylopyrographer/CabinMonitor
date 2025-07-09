#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "hw_config.h"
#include "camera.h"
#include "sensors.h"
#include "storage.h"
#include "cellular.h"
#include "google_drive.h"
#include "led_control.h"
#include "time_sync.h"
#include "provisioning.h"
#include "ota.h"
#include "button_control.h"
#include "sms_messaging.h"
#include <LittleFS.h>

// Global state
enum SystemState {
  STATE_INIT,
  STATE_NEEDS_PROVISIONING,
  STATE_PROVISIONING,
  STATE_IDLE,
  STATE_MOTION_DETECTED,
  STATE_SOUND_DETECTED,
  STATE_CAPTURING,
  STATE_UPLOADING,
  STATE_OTA_UPDATE,
  STATE_FACTORY_RESET,
  STATE_MONITORING_DISABLED,
  STATE_ERROR
};

SystemState currentState = STATE_INIT;
unsigned long lastActivityTime = 0;
unsigned long lastCaptureTime = 0;
unsigned long lastSyncTime = 0;
unsigned long lastGDriveCheckTime = 0;
unsigned long weeklyPhotoCheckTime = 0;
unsigned long minuteCounterTime = 0;
bool gdriveCommOK = false;
int minuteCounter = 0;
bool driveCheckNeeded = true;
bool otaRequested = false;
bool factoryResetRequested = false;
bool provisioningRequested = false;
bool uploadInterrupted = false;
bool isProvisioned = false;

// Function prototypes
void checkSensors();
void handleStateMachine();
void checkTimeEvents();
bool checkWeeklyPhotoTime();
bool checkDailyDriveCheckTime();
void captureAndSavePhoto();
void checkButton();
void setupFromScratch();
void factoryReset();
bool checkProvisioned();

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-S3 Monitoring Device Starting...");
  
  // Initialize LED first for status indication
  initLED();
  setLEDState(LED_INIT);
  
  // Initialize button control
  initButton();
  
  // Check button state for special actions
  ButtonAction startupAction = checkStartupButtonPress();
  
  switch (startupAction) {
    case BUTTON_FACTORY_RESET:
      factoryResetRequested = true;
      currentState = STATE_FACTORY_RESET;
      setLEDState(LED_FACTORY_RESET);
      break;
      
    case BUTTON_OTA_UPDATE:
      otaRequested = true;
      currentState = STATE_OTA_UPDATE;
      setLEDState(LED_OTA_MODE);
      break;
      
    case BUTTON_PROVISIONING:
      provisioningRequested = true;
      currentState = STATE_PROVISIONING;
      setLEDState(LED_PROVISIONING);
      break;
      
    case BUTTON_NORMAL_START:
    default:
      // Continue with normal startup
      break;
  }
  
  // Handle special states first
  if (factoryResetRequested) {
    factoryReset();
    return;
  }
  
  if (otaRequested) {
    initOTA();
    return;
  }
  
  // Check if the device has been provisioned
  isProvisioned = checkProvisioned();
  
  if (provisioningRequested || !isProvisioned) {
    setupFromScratch();
    return;
  }
  
  // Normal initialization
  if (!initStorage()) {
    Serial.println("Storage initialization failed!");
    currentState = STATE_ERROR;
    setLEDState(LED_ERROR);
    return;
  }
  
  if (!initCamera()) {
    Serial.println("Camera initialization failed!");
    currentState = STATE_ERROR;
    setLEDState(LED_ERROR);
    return;
  }
  
  if (!initSensors()) {
    Serial.println("Sensors initialization failed!");
    currentState = STATE_ERROR;
    setLEDState(LED_ERROR);
    return;
  }
  
  if (!initCellular()) {
    Serial.println("Cellular initialization failed!");
    // Not critical, will retry later
    Serial.println("Will retry cellular connection later");
  }
  
  // Initialize SMS messaging
  if (!initSMSMessaging()) {
    Serial.println("SMS messaging not configured, will use defaults");
  }
  
  // Try to sync time
  if (syncTimeWithNTP()) {
    lastSyncTime = millis();
    Serial.println("Time synchronized successfully");
  } else {
    Serial.println("Time sync failed, will retry later");
  }
  
  // Check Google Drive connectivity (returns false if not configured)
  if (isGoogleDriveConfigured()) {
    // Initialize Google Drive API
    if (initGoogleDrive()) {
      Serial.println("Google Drive initialized successfully");
      
      // Check for unsent files on SD
      int fileCount = getFileCount();
      if (fileCount > 0) {
        Serial.printf("Found %d unsent files, starting upload\n", fileCount);
        currentState = STATE_UPLOADING;
        setLEDState(LED_UPLOADING);
        sendActivityDetectedSMS();
      } else {
        // Everything initialized successfully
        currentState = STATE_IDLE;
        setLEDState(LED_IDLE);
      }
    } else {
      Serial.println("Google Drive initialization failed, entering provisioning mode");
      setupFromScratch();
      return;
    }
  } else {
    Serial.println("Google Drive not configured, entering provisioning mode");
    setupFromScratch();
    return;
  }
  
  Serial.println("Initialization complete, entering main loop");
  weeklyPhotoCheckTime = millis();
  lastGDriveCheckTime = millis();
  minuteCounterTime = millis();
}

void loop() {
  // Handle special states directly
  if (factoryResetRequested) {
    // Just wait forever after factory reset
    delay(1000);
    return;
  }
  
  if (otaRequested) {
    handleOTA();
    updateLED(); // Update LED for blinking effects
    return;
  }
  
  if (currentState == STATE_PROVISIONING) {
    handleProvisioning();
    updateLED(); // Update LED for blinking effects
    
    // Check if provisioning is complete
    if (!isProvisioningActive() && isGoogleDriveConfigured()) {
      Serial.println("Provisioning complete, restarting device");
      delay(1000);
      ESP.restart();
    }
    return;
  }
  
  // Handle the current state
  handleStateMachine();
  
  // Check time-based events
  checkTimeEvents();
  
  // Check button for runtime actions
  checkButton();
  
  // Update LED for blinking effects
  updateLED();
  
  // Small delay to prevent CPU hogging
  delay(10);
}

void handleStateMachine() {
  switch (currentState) {
    case STATE_IDLE:
      if (isMonitoringEnabled()) {
        checkSensors();
      }
      break;
      
    case STATE_MOTION_DETECTED:
    case STATE_SOUND_DETECTED:
      // Prepare for capture
      lastActivityTime = millis();
      currentState = STATE_CAPTURING;
      setLEDState(LED_CAPTURING);
      break;
      
    case STATE_CAPTURING:
      // Check if we should still be capturing
      if (millis() - lastActivityTime > INACTIVITY_TIMEOUT_MS) {
        // No activity for a while, stop capturing and start uploading
        Serial.println("Inactivity timeout reached, starting upload");
        currentState = STATE_UPLOADING;
        setLEDState(LED_UPLOADING);
        // Send SMS notification for activity detection
        sendActivityDetectedSMS();
        break;
      }
      
      // Check for new activity if monitoring is enabled
      if (isMonitoringEnabled()) {
        checkSensors();
      }
      
      // Capture photos at the defined interval
      if (millis() - lastCaptureTime > CAPTURE_INTERVAL_MS) {
        captureAndSavePhoto();
        lastCaptureTime = millis();
      }
      break;
      
    case STATE_UPLOADING:
      // Check if monitoring is enabled and if new activity is detected
      if (isMonitoringEnabled()) {
        if (isPIRTriggered() || isSoundDetected()) {
          // New activity detected, return to capturing
          Serial.println("Activity detected during upload, resuming capture");
          currentState = STATE_CAPTURING;
          setLEDState(LED_CAPTURING);
          lastActivityTime = millis();
          uploadInterrupted = true;
          break;
        }
      }
      
      // Check if we have connectivity to Google Drive
      if (!gdriveCommOK && driveCheckNeeded) {
        // Try to check connectivity first
        if (isGoogleDriveConfigured() && initGoogleDrive()) {
          gdriveCommOK = true;
          sendCommunicationOkSMS();
        } else {
          gdriveCommOK = false;
          sendNoCommunicationSMS();
        }
        driveCheckNeeded = false;
      }
      
      // Only proceed with upload if we have connectivity
      if (gdriveCommOK) {
        // Upload files and delete after successful upload
        if (uploadFilesToGoogleDrive()) {
          Serial.println("All files uploaded successfully");
          deleteAllFiles();
          uploadInterrupted = false;
          currentState = STATE_IDLE;
          setLEDState(LED_IDLE);
        } else {
          Serial.println("Upload failed or was interrupted, will retry later");
          // For now, just go back to idle and retry later
          currentState = STATE_IDLE;
          setLEDState(LED_IDLE);
        }
      } else {
        // No Google Drive connectivity, go back to idle
        Serial.println("No Google Drive connectivity, skipping upload");
        currentState = STATE_IDLE;
        setLEDState(LED_IDLE);
      }
      break;
      
    case STATE_MONITORING_DISABLED:
      // Just wait for button action to re-enable
      if (isMonitoringEnabled()) {
        // Monitoring has been re-enabled
        Serial.println("Monitoring re-enabled");
        currentState = STATE_IDLE;
        setLEDState(LED_IDLE);
      }
      break;
      
    case STATE_ERROR:
      // Stay in error state, maybe implement a recovery mechanism
      delay(1000); // Prevent fast looping in error state
      break;
      
    default:
      // Should not get here
      Serial.println("Unknown state!");
      currentState = STATE_ERROR;
      setLEDState(LED_ERROR);
      break;
  }
}

void checkSensors() {
  // Check PIR sensor
  if (isPIRTriggered()) {
    Serial.println("Motion detected by PIR sensor");
    currentState = STATE_MOTION_DETECTED;
    setLEDState(LED_PIR_DETECTED);
    lastActivityTime = millis();
  }
  
  // Check sound level
  if (isSoundDetected()) {
    Serial.println("Sound detected by MEMS microphone");
    currentState = STATE_SOUND_DETECTED;
    setLEDState(LED_SOUND_DETECTED);
    lastActivityTime = millis();
  }
}

void checkTimeEvents() {
  unsigned long currentTime = millis();
  
  // Check if we need to sync time (once a day)
  if (currentTime - lastSyncTime > NTP_UPDATE_INTERVAL_MS) {
    Serial.println("Performing daily time sync");
    if (syncTimeWithNTP()) {
      lastSyncTime = currentTime;
      Serial.println("Time synchronized successfully");
    } else {
      Serial.println("Time sync failed, will retry later");
    }
  }
  
  // Check if we need to check Google Drive connectivity (once a day)
  if (checkDailyDriveCheckTime() || (currentTime - lastGDriveCheckTime > NTP_UPDATE_INTERVAL_MS)) {
    Serial.println("Performing daily Google Drive connectivity check");
    
    if (isGoogleDriveConfigured() && initGoogleDrive()) {
      gdriveCommOK = true;
      sendCommunicationOkSMS();
      Serial.println("Google Drive connectivity OK");
    } else {
      gdriveCommOK = false;
      sendNoCommunicationSMS();
      Serial.println("Failed to connect to Google Drive");
    }
    
    lastGDriveCheckTime = currentTime;
  }
  
  // Check if we need to take a weekly photo (if no activity)
  if (currentTime - weeklyPhotoCheckTime > 60000) { // Check once per minute
    weeklyPhotoCheckTime = currentTime;
    
    if (checkWeeklyPhotoTime() && currentState == STATE_IDLE && isMonitoringEnabled()) {
      Serial.println("Taking weekly photo (no activity detected)");
      captureAndSavePhoto();
      
      // Send "no activity" SMS
      sendNoActivityDetectedSMS();
      
      // Upload the weekly photo
      currentState = STATE_UPLOADING;
      setLEDState(LED_UPLOADING);
    }
  }
  
  // Update minute counter for delayed events
  if (currentTime - minuteCounterTime > 60000) { // Every minute
    minuteCounter++;
    minuteCounterTime = currentTime;
    
    if (currentState == STATE_MONITORING_DISABLED) {
      updateLED(); // Ensure the double blink happens every minute
    }
  }
}

bool checkWeeklyPhotoTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return false;
  }
  
  // Check if it's the right day and time for weekly photo
  return (timeinfo.tm_wday == WEEKLY_PHOTO_DAY && 
          timeinfo.tm_hour == WEEKLY_PHOTO_HOUR && 
          timeinfo.tm_min == WEEKLY_PHOTO_MINUTE);
}

bool checkDailyDriveCheckTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return false;
  }
  
  // Check if it's the right time for daily Google Drive check
  return (timeinfo.tm_hour == GDRIVE_CHECK_HOUR && 
          timeinfo.tm_min == GDRIVE_CHECK_MINUTE);
}

void captureAndSavePhoto() {
  // Check light level and adjust IR LEDs and IR cut filter
  int lightLevel = getLightLevel();
  
  // Toggle IR LEDs based on light level
  if (lightLevel < LIGHT_THRESHOLD_IR_ENABLE) {
    enableIRLEDs(true);
  } else {
    enableIRLEDs(false);
  }
  
  // Toggle IR cut filter based on light level
  if (lightLevel < LIGHT_THRESHOLD_IR_CUT) {
    enableIRCut(false); // Disable IR cut (allow IR light)
  } else {
    enableIRCut(true);  // Enable IR cut (block IR light)
  }
  
  // Get current time for filename
  char timestamp[20];
  getTimestampString(timestamp, sizeof(timestamp));
  
  // Get base filename from preferences (or use default)
  String baseFilename = getBaseFilename();
  
  // Create filename
  char filename[50];
  snprintf(filename, sizeof(filename), "/%s_%s.jpg", baseFilename.c_str(), timestamp);
  
  // Capture photo
  camera_fb_t* fb = capturePhoto();
  if (!fb) {
    Serial.println("Failed to capture photo");
    return;
  }
  
  // Save to SD card
  if (!savePhotoToSD(filename, fb->buf, fb->len)) {
    Serial.println("Failed to save photo to SD card");
  } else {
    Serial.printf("Photo saved: %s\n", filename);
  }
  
  // Return the frame buffer back to the camera
  returnPhotoBuffer(fb);
}

void checkButton() {
  // Process button actions
  ButtonAction action = handleButton();
  
  switch (action) {
    case BUTTON_DISABLE_MONITORING:
      Serial.println("Triple-click detected: Disabling monitoring");
      currentState = STATE_MONITORING_DISABLED;
      setLEDState(LED_MONITORING_DISABLED);
      sendMonitoringDisabledSMS();
      break;
      
    case BUTTON_RESUME_MONITORING:
      Serial.println("Triple-click detected: Enabling monitoring (with 20-minute delay)");
      // The actual state change will happen after the timeout
      sendMonitoringEnabledSMS();
      break;
      
    case BUTTON_NO_ACTION:
    default:
      // No action needed
      break;
  }
}

void setupFromScratch() {
  // Initialize storage for provisioning
  if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS for provisioning");
    currentState = STATE_ERROR;
    setLEDState(LED_ERROR);
    return;
  }
  
  // Enter provisioning mode
  currentState = STATE_PROVISIONING;
  setLEDState(LED_PROVISIONING);
  startProvisioning();
  
  Serial.println("Waiting for provisioning...");
}

void factoryReset() {
  Serial.println("Performing factory reset...");
  
  // Clear all settings from preferences
  Preferences preferences;
  preferences.begin("monitoring", false);
  preferences.clear();
  preferences.end();
  
  // Clear Google Drive credentials
  preferences.begin("gdrive", false);
  preferences.clear();
  preferences.end();
  
  // Clear SMS settings
  preferences.begin("sms", false);
  preferences.clear();
  preferences.end();
  
  // Clear storage settings
  preferences.begin("storage", false);
  preferences.clear();
  preferences.end();
  
  // Additional cleanup if needed
  if (LittleFS.begin(true)) {
    LittleFS.format();
  }
  
  Serial.println("Factory reset complete. Halting system.");
  setLEDState(LED_ERROR);
  // Halt system - requires a manual reset
  while (true) {
    updateLED();
    delay(100);
  }
}

bool checkProvisioned() {
  Preferences preferences;
  if (preferences.begin("monitoring", true)) {
    bool provisioned = preferences.getBool("provisioned", false);
    preferences.end();
    return provisioned;
  }
  return false;
}