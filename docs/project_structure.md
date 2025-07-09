# ESP32-S3 Monitoring Device

## Project Structure
```
└── monitoring-device/
    ├── platformio.ini                  // PlatformIO configuration with LittleFS settings
    ├── src/
    │   ├── main.cpp                    // Main application entry point
    │   ├── config.h                    // Configuration parameters
    │   ├── hw_config.h                 // Hardware pin definitions
    │   ├── camera.cpp/.h               // Camera handling
    │   ├── sensors.cpp/.h              // PIR, microphone, light sensor management
    │   ├── storage.cpp/.h              // SD card and LittleFS operations
    │   ├── cellular.cpp/.h             // SIM7000G modem functions
    │   ├── google_drive.cpp/.h         // Google Drive API interactions
    │   ├── led_control.cpp/.h          // WS2812 LED status indicators
    │   ├── time_sync.cpp/.h            // NTP time synchronization
    │   ├── provisioning.cpp/.h         // WiFi/BLE provisioning 
    │   ├── ota.cpp/.h                  // OTA update handling
    │   ├── button_control.cpp/.h       // Button actions with XP_Button library
    │   └── sms_messaging.cpp/.h        // SMS notification system
    └── data/                           // Files to be uploaded to LittleFS
        ├── index.html                  // Web UI for provisioning
        └── ota.html                    // Web UI for OTA updates
```

## Key Components

### LittleFS File System
The device uses LittleFS instead of SPIFFS for file system operations, providing better wear leveling, higher efficiency, and improved reliability.

### Button Control System
Uses the XP_Button library to detect various button interactions:
- Long press (1-2s): Enter provisioning mode
- Longer press (2-3s): Enter OTA update mode
- Very long press (10s+): Factory reset
- Triple-click: Toggle monitoring state

### SMS Notification System
Comprehensive SMS notification system that sends alerts for:
- Activity detection
- No activity (weekly)
- Google Drive connectivity status
- Monitoring state changes

### Monitoring Control
Ability to enable/disable monitoring via triple-click:
- When disabled: LED double-blinks in red every minute
- 20-minute delay before re-enabling after triple-click

### Enhanced Provisioning
Multi-tab web interface for configuring:
- Google Drive credentials
- Device thresholds and parameters
- SMS notifications settings and messages
