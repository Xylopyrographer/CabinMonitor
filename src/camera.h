#ifndef CAMERA_H
#define CAMERA_H

#include <Arduino.h>
#include "esp_camera.h"

// Initialize the camera
bool initCamera();

// Capture a photo and return the frame buffer
camera_fb_t* capturePhoto();

// Return the frame buffer to the camera
void returnPhotoBuffer(camera_fb_t* fb);

#endif // CAMERA_H
