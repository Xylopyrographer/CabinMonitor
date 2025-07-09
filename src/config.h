#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Device settings
#define DEVICE_NAME "ESP32S3-Monitor"
#define FIRMWARE_VERSION "1.1.0"

// Monitoring settings
#define PIR_COOLDOWN_MS              5000    // Cooldown period after PIR trigger (ms)
#define SOUND_DETECTION_THRESHOLD    3000    // Sound level threshold for triggering
#define SOUND_SAMPLING_WINDOW_MS     500     // Period to sample sound level (ms)
#define LIGHT_THRESHOLD_IR_ENABLE    20      // Light level threshold to enable IR LEDs (0-100)
#define LIGHT_THRESHOLD_IR_CUT       30      // Light level threshold to enable IR cut (0-100)
#define INACTIVITY_TIMEOUT_MS        60000   // Time without activity before stopping capture (ms)
#define CAPTURE_INTERVAL_MS          2000    // Time between consecutive photo captures (ms)

// Button settings
#define TRIPLE_CLICK_WINDOW_MS       3000    // Time window for triple click detection
#define MONITORING_RESUME_DELAY_MS   1200000 // 20 minutes wait after re-enabling monitoring

// Camera settings
#define CAMERA_FRAME_SIZE            FRAMESIZE_UXGA  // Can be: FRAMESIZE_QQVGA to FRAMESIZE_UXGA
#define CAMERA_JPEG_QUALITY          10              // 0-63, lower means higher quality
#define CAMERA_BRIGHTNESS            0               // -2 to 2
#define CAMERA_CONTRAST              0               // -2 to 2
#define CAMERA_SATURATION            0               // -2 to 2
#define CAMERA_SPECIAL_EFFECT        0               // 0=None, 1=Negative, 2=Grayscale, etc.
#define CAMERA_HORIZONTAL_MIRROR     false
#define CAMERA_VERTICAL_FLIP         false

// Storage settings
#define BASE_FILENAME               "capture"
#define MAX_FILES_PER_SESSION       100             // Maximum files to store before forced upload
#define SD_CHECK_INTERVAL_MS        60000           // Time between SD card space checks
#define MIN_SD_FREE_SPACE_MB        100             // Minimum free space required on SD

// Time sync settings
#define NTP_SERVER                  "pool.ntp.org"
#define NTP_TIMEZONE_OFFSET_SEC     0               // Change according to your timezone
#define NTP_UPDATE_INTERVAL_MS      86400000        // 24 hours
#define TIME_FORMAT                 "%Y%m%d_%H%M%S" // Format for filename timestamp

// Weekly photo settings
#define WEEKLY_PHOTO_DAY            1               // Day of week (0=Sunday, 1=Monday, etc.)
#define WEEKLY_PHOTO_HOUR           12              // Hour of day (0-23)
#define WEEKLY_PHOTO_MINUTE         0               // Minute of hour (0-59)

// Daily Google Drive check
#define GDRIVE_CHECK_HOUR           9               // Hour to check Google Drive connectivity
#define GDRIVE_CHECK_MINUTE         0               // Minute to check Google Drive connectivity

// Network settings
#define BLE_SERVICE_UUID            "8baf6fe2-9e32-11ed-a1eb-0242ac120002"
#define BLE_CHARACTERISTIC_UUID     "8baf7230-9e32-11ed-a1eb-0242ac120002"
#define WIFI_AP_PASSWORD            "monitor123"
#define WIFI_AP_CHANNEL             1
#define WIFI_AP_MAX_CONNECTIONS     2
#define PROVISIONING_TIMEOUT_MS     300000          // 5 minutes

// Cellular settings
#define APN_NAME                    "your-apn-name" // Set your cellular APN
#define CELLULAR_TIMEOUT_MS         60000           // Timeout for cellular operations
#define CELLULAR_RETRY_COUNT        3               // Number of retries for cellular operations

// SMS settings
#define SMS_PHONE_MAX_LENGTH        13              // Max length of phone number
#define SMS_MESSAGE_MIN_LENGTH      3               // Min length of SMS messages
#define SMS_MESSAGE_MAX_LENGTH      80              // Max length of SMS messages

// OTA settings
#define OTA_TIMEOUT_MS              300000          // 5 minutes timeout for OTA

// LED settings
#define LED_BRIGHTNESS              50              // 0-255
#define LED_BLINK_INTERVAL_MS       500             // Blink interval for error indication

#endif // CONFIG_H
