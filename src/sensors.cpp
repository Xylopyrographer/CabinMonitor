#include "sensors.h"
#include "hw_config.h"
#include "config.h"

// Global variables for sensors
unsigned long lastPIRTriggerTime = 0;
int16_t* i2s_buffer = NULL;
size_t i2s_buffer_size = 0;
bool i2s_initialized = false;

bool initSensors() {
  // Initialize PIR sensor
  pinMode(PIR_SENSOR_PIN, INPUT);
  
  // Initialize light sensor
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  
  // Initialize IR LED and IR cut control
  pinMode(IR_LED_PIN, OUTPUT);
  pinMode(IR_CUT_PIN, OUTPUT);
  digitalWrite(IR_LED_PIN, LOW);  // IR LEDs off by default
  digitalWrite(IR_CUT_PIN, HIGH); // IR cut enabled by default (blocks IR)
  
  // Initialize I2S for MEMS microphone
  esp_err_t err;
  
  // Allocate buffer for I2S data
  i2s_buffer_size = 512;
  i2s_buffer = (int16_t*)malloc(i2s_buffer_size * sizeof(int16_t));
  if (i2s_buffer == NULL) {
    Serial.println("Failed to allocate I2S buffer");
    return false;
  }
  
  // Configure I2S for MEMS microphone
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD_PIN
  };
  
  err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.println("Failed to install I2S driver");
    free(i2s_buffer);
    i2s_buffer = NULL;
    return false;
  }
  
  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    Serial.println("Failed to set I2S pins");
    i2s_driver_uninstall(I2S_PORT);
    free(i2s_buffer);
    i2s_buffer = NULL;
    return false;
  }
  
  i2s_initialized = true;
  Serial.println("Sensors initialized successfully");
  return true;
}

bool isPIRTriggered() {
  unsigned long currentTime = millis();
  
  // Read PIR sensor
  if (digitalRead(PIR_SENSOR_PIN) == HIGH) {
    // Check if we're past the cooldown period
    if (currentTime - lastPIRTriggerTime > PIR_COOLDOWN_MS) {
      lastPIRTriggerTime = currentTime;
      return true;
    }
  }
  
  return false;
}

int16_t getSoundLevel() {
  if (!i2s_initialized || i2s_buffer == NULL) {
    return 0;
  }
  
  size_t bytes_read = 0;
  esp_err_t err = i2s_read(I2S_PORT, i2s_buffer, i2s_buffer_size * sizeof(int16_t), &bytes_read, portMAX_DELAY);
  
  if (err != ESP_OK) {
    Serial.println("Error reading from I2S");
    return 0;
  }
  
  // Process audio to get sound level (simple amplitude calculation)
  int samples_read = bytes_read / sizeof(int16_t);
  int32_t sum = 0;
  int16_t abs_sample;
  int16_t max_amplitude = 0;
  
  for (int i = 0; i < samples_read; i++) {
    abs_sample = abs(i2s_buffer[i]);
    if (abs_sample > max_amplitude) {
      max_amplitude = abs_sample;
    }
    sum += abs_sample;
  }
  
  // Return the average amplitude
  return (samples_read > 0) ? (sum / samples_read) : 0;
}

bool isSoundDetected() {
  // Get current sound level
  int16_t soundLevel = getSoundLevel();
  
  // Compare with threshold
  return (soundLevel > SOUND_DETECTION_THRESHOLD);
}

int getLightLevel() {
  // Read analog value from photo resistor
  int rawValue = analogRead(LIGHT_SENSOR_PIN);
  
  // Map the raw value (0-4095 for ESP32) to a 0-100 scale
  // Adjust these values based on your specific photo resistor and lighting conditions
  int mappedValue = map(rawValue, 0, 4095, 0, 100);
  
  // Invert the value if needed (depending on your circuit)
  // If your photo resistor is connected to pull-up, lower values mean more light
  // In this case, we assume lower values mean less light
  
  return mappedValue;
}

void enableIRLEDs(bool enable) {
  digitalWrite(IR_LED_PIN, enable ? HIGH : LOW);
}

void enableIRCut(bool enable) {
  // IR cut enabled (HIGH) blocks IR light
  // IR cut disabled (LOW) allows IR light to pass
  digitalWrite(IR_CUT_PIN, enable ? HIGH : LOW);
}
