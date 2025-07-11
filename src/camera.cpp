#include "camera.h"
#include "hw_config.h"
#include "config.h"

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  
  // Initialize with high quality and lower resolution first
  config.frame_size = CAMERA_FRAME_SIZE;
  config.jpeg_quality = CAMERA_JPEG_QUALITY;
  config.fb_count = 2;
  
  // Check if PSRAM is enabled - if not, limit frame size
  if (!psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_count = 1;
    Serial.println("PSRAM not found, limiting frame size");
  }
  
  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }
  
  // Apply additional camera settings
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, CAMERA_BRIGHTNESS);
    s->set_contrast(s, CAMERA_CONTRAST);
    s->set_saturation(s, CAMERA_SATURATION);
    s->set_special_effect(s, CAMERA_SPECIAL_EFFECT);
    s->set_hmirror(s, CAMERA_HORIZONTAL_MIRROR);
    s->set_vflip(s, CAMERA_VERTICAL_FLIP);
  }
  
  Serial.println("Camera initialized successfully");
  return true;
}

camera_fb_t* capturePhoto() {
  // Take a photo
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return NULL;
  }
  
  return fb;
}

void returnPhotoBuffer(camera_fb_t* fb) {
  if (fb) {
    esp_camera_fb_return(fb);
  }
}
