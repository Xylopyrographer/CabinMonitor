#ifndef HW_CONFIG_H
#define HW_CONFIG_H

// Camera pins for ESP32-S3
#define CAMERA_MODEL_ESP32S3_EYE
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     40
#define SIOD_GPIO_NUM     17
#define SIOC_GPIO_NUM     18
#define Y9_GPIO_NUM       39
#define Y8_GPIO_NUM       41
#define Y7_GPIO_NUM       42
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       11
#define Y4_GPIO_NUM       10
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM       8
#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     13

// IR Cut Module
#define IR_CUT_PIN        5

// IR LED Array
#define IR_LED_PIN        4

// I2S MEMS Microphone
#define I2S_SCK_PIN       47
#define I2S_WS_PIN        48
#define I2S_SD_PIN        21
#define I2S_PORT          I2S_NUM_0

// PIR Sensor
#define PIR_SENSOR_PIN    14

// Photo Resistor (Light Sensor)
#define LIGHT_SENSOR_PIN  1

// WS2812 RGB LED
#define RGB_LED_PIN       16
#define NUM_LEDS          1

// SD Card Module
#define SD_CS_PIN         34
#define SD_MOSI_PIN       35
#define SD_MISO_PIN       37
#define SD_SCK_PIN        36

// SIM7000G Cellular Module
#define SIM7000_RX_PIN    43
#define SIM7000_TX_PIN    44
#define SIM7000_RST_PIN   2
#define SIM7000_PWR_PIN   3

// Boot mode selection button
#define BOOT_MODE_PIN     0  // Usually GPIO0 is used as boot mode selection

// LED state colors
#define LED_COLOR_IDLE           0x001100  // Dim green
#define LED_COLOR_PIR_DETECTED   0xFF0000  // Red
#define LED_COLOR_SOUND_DETECTED 0xFFFF00  // Yellow
#define LED_COLOR_CAPTURING      0x0000FF  // Blue
#define LED_COLOR_UPLOADING      0xFF00FF  // Purple
#define LED_COLOR_OTA_MODE       0x00FFFF  // Cyan
#define LED_COLOR_ERROR          0xFF0000  // Red (blinking)
#define LED_COLOR_PROVISIONING   0x00FF00  // Green (blinking)
#define LED_COLOR_BUTTON_PRESSED 0xFFFFFF  // White
#define LED_COLOR_FACTORY_RESET  0xFF4000  // Orange
#define LED_COLOR_MONITORING_DISABLED 0xFF0000 // Red (double blink)

// LED states enum (moved from led_control.h to keep all state definitions together)
typedef enum {
  LED_INIT,
  LED_IDLE,
  LED_PIR_DETECTED,
  LED_SOUND_DETECTED,
  LED_CAPTURING,
  LED_UPLOADING,
  LED_OTA_MODE,
  LED_ERROR,
  LED_PROVISIONING,
  LED_BUTTON_PRESSED,
  LED_FACTORY_RESET,
  LED_MONITORING_DISABLED
} LEDState;

#endif // HW_CONFIG_H
