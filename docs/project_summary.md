# ESP32-S3 Monitoring Device - Updated Project Summary

## Overview
This project creates a surveillance/monitoring device based on ESP32-S3 that:

1. Monitors for motion (PIR) and sound (MEMS microphone) 
2. Captures images when activity is detected
3. Automatically uploads captured images to Google Drive
4. Supports weekly automatic image capture if no activity
5. Provides visual status indication via WS2812 RGB LED
6. Includes full provisioning capability (via WiFi/BLE)
7. Supports OTA (Over-the-Air) firmware updates
8. Provides SMS notifications for key events
9. Includes button control for device state management
10. Allows monitoring to be enabled/disabled

## Hardware Components
- ESP32-S3 microcontroller
- OV5460 camera with IR cut module
- SD card module
- I2S MEMS microphone
- PIR motion sensor
- IR LED array for night vision
- Photoresistor for ambient light sensing
- WS2812 RGB LED for status indication
- SIM7000G cellular modem for image upload and SMS
- Boot mode selection button

## System Architecture

### File System
- Uses LittleFS instead of SPIFFS for improved wear-leveling and reliability
- SD card for storing captured images

### Button Control
- Triple-click detection for toggling monitoring state
- Long-press detection for entering special modes:
  - 1-2 seconds: Enter provisioning mode
  - 2-3 seconds: Enter OTA update mode
  - 10+ seconds: Perform factory reset

### Provisioning
- WiFi/BLE-based device setup
- Configurable parameters:
  - Google Drive credentials
  - Light and sound thresholds
  - Base filename for captures
  - SMS notification settings

### Notification System
- SMS notifications for:
  - Activity detection
  - No activity (weekly report)
  - Google Drive connectivity status
  - Monitoring state changes

### LED Indicator
- Visual status indication with different colors and patterns:
  - Idle: Dim green
  - Motion detected: Red
  - Sound detected: Yellow
  - Capturing: Blue
  - Uploading: Purple
  - OTA/Provisioning Mode: Cyan
  - Error: Blinking red
  - Button Pressed: White
  - Factory Reset: Orange
  - Monitoring Disabled: Red (double blink)

### Monitoring Control
- Enable/disable via triple-click button action
- 20-minute delay before re-enabling after disabling
- SMS notifications for state changes

## Implementation Details

### File System Management
- LittleFS for configuration data and web interfaces
- SD card for image storage
- Automatic cleanup after successful uploads

### Camera Management
- Automatic IR LED and IR-cut filter control based on ambient light
- Configurable capture interval and quality settings

### Connectivity
- Daily Google Drive connectivity checks with SMS notifications
- Daily NTP time synchronization
- Weekly activity status with photo capture

### Error Handling
- Visual indication of error states
- Automatic recovery mechanisms
- SMS notifications for critical issues

## Deployment Instructions

### Software Requirements
- PlatformIO development environment
- Required libraries (see platformio.ini):
  - Adafruit NeoPixel
  - ArduinoJson
  - ESP32 Camera Driver
  - ESP-Google-Drive-API
  - ESP Google OAuth
  - LittleFS_esp32
  - XP_Button
  - SoftwareSerial

### Setup Process
1. Flash firmware via PlatformIO
2. Upload LittleFS data files:
   - PlatformIO → Project Tasks → Platform → Upload Filesystem Image
3. Enter provisioning mode (press button for 1-2 seconds on boot)
4. Connect to provisioning WiFi AP or use BLE
5. Configure Google Drive credentials and other settings
6. Device will restart and begin normal operation

### Button Controls
- Normal boot: No button press
- Provisioning mode: 1-2 second press during boot
- OTA update mode: 2-3 second press during boot
- Factory reset: 10+ second press during boot
- Toggle monitoring: Triple-click while running

### LED Status Codes
- Solid colors indicate normal operation states
- Blinking indicates special states or errors
- Double-blinking red indicates monitoring disabled

## Maintenance and Troubleshooting

### OTA Updates
1. Enter OTA mode (2-3 second button press during boot)
2. Connect to OTA WiFi AP
3. Use web interface to upload new firmware

### Factory Reset
1. Enter factory reset mode (10+ second button press during boot)
2. All settings and credentials will be erased
3. Device will halt (power cycle to restart in provisioning mode)

### Troubleshooting
- Check LED status code for quick diagnosis
- SD card issues: Format to FAT32, check connections
- Cellular connectivity: Verify SIM status and APN settings
- Google Drive issues: Reauthorize and refresh tokens
