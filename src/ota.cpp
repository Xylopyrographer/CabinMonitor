#include "ota.h"
#include "led_control.h"
#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <LittleFS.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// OTA Web Server
WebServer otaServer(80);
bool otaWifiActive = false;
unsigned long otaStartTime = 0;

// OTA BLE
BLEServer *otaServer = NULL;
BLECharacteristic *otaCharacteristic = NULL;
bool otaBleActive = false;
bool otaDeviceConnected = false;
uint32_t otaBufferSize = 0;
uint32_t otaCurrentSize = 0;
bool otaUpdateStarted = false;

class OTAServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* server) {
    otaDeviceConnected = true;
    Serial.println("BLE Client connected for OTA");
  }
  
  void onDisconnect(BLEServer* server) {
    otaDeviceConnected = false;
    Serial.println("BLE Client disconnected from OTA");
    // Restart advertising
    BLEDevice::startAdvertising();
  }
};

class OTACharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      // Process OTA data packet
      processOTAData((uint8_t*)value.data(), value.length());
    }
  }
};

// Forward declarations of functions
void setupOTAWiFi();
void setupOTAWebServer();
void setupOTABLE();
void handleOTARoot();
void handleOTAUpdate();
void handleOTADoUpdate();
void processOTAData(uint8_t* data, size_t len);

bool initOTA() {
  Serial.println("Initializing OTA update mode");
  
  // Set LED to indicate OTA mode
  setLEDState(LED_OTA_MODE);
  
  // Initialize LittleFS for web files
  if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS for OTA");
    return false;
  }
  
  // Start both WiFi AP and BLE for OTA
  setupOTAWiFi();
  setupOTABLE();
  
  otaStartTime = millis();
  Serial.println("OTA update mode initialized");
  return true;
}

void handleOTA() {
  // Handle WiFi OTA server requests
  if (otaWifiActive) {
    otaServer.handleClient();
  }
  
  // For BLE OTA, we use callbacks so no explicit handling needed here
  
  // Check for timeout
  if (millis() - otaStartTime > OTA_TIMEOUT_MS) {
    Serial.println("OTA timeout reached, restarting device");
    delay(1000);
    ESP.restart();
  }
}

void setupOTAWiFi() {
  // Set up WiFi in AP mode for OTA
  WiFi.mode(WIFI_AP);
  
  String ssid = String(DEVICE_NAME) + "-OTA-" + String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFF), HEX);
  
  if (WiFi.softAP(ssid.c_str(), WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONNECTIONS)) {
    Serial.println("OTA WiFi AP started: " + ssid);
    Serial.println("IP address: " + WiFi.softAPIP().toString());
    
    // Start OTA web server
    setupOTAWebServer();
    otaWifiActive = true;
  } else {
    Serial.println("Failed to start OTA WiFi AP");
  }
}

void setupOTAWebServer() {
  // Set up web server routes for OTA
  otaServer.on("/", handleOTARoot);
  otaServer.on("/update", HTTP_POST, handleOTAUpdate, handleOTADoUpdate);
  
  // Start server
  otaServer.begin();
  Serial.println("OTA Web server started");
}

void setupOTABLE() {
  // Initialize BLE for OTA
  BLEDevice::init(String(DEVICE_NAME + "-OTA").c_str());
  
  // Create BLE server
  otaServer = BLEDevice::createServer();
  otaServer->setCallbacks(new OTAServerCallbacks());
  
  // Create BLE service for OTA
  BLEService *pService = otaServer->createService("0000FFFF-0000-1000-8000-00805F9B34FB"); // Custom OTA service UUID
  
  // Create BLE characteristic for OTA data
  otaCharacteristic = pService->createCharacteristic(
                      "0000FFFE-0000-1000-8000-00805F9B34FB", // Custom OTA characteristic UUID
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  
  otaCharacteristic->setCallbacks(new OTACharacteristicCallbacks());
  otaCharacteristic->addDescriptor(new BLE2902());
  
  // Create BLE characteristic for OTA control (start, size, end)
  BLECharacteristic *otaControlChar = pService->createCharacteristic(
                      "0000FFFD-0000-1000-8000-00805F9B34FB", // Custom OTA control characteristic UUID
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  
  otaControlChar->setCallbacks(new OTACharacteristicCallbacks());
  otaControlChar->addDescriptor(new BLE2902());
  
  // Start the service
  pService->start();
  
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID("0000FFFF-0000-1000-8000-00805F9B34FB");
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE OTA advertising started");
  otaBleActive = true;
}

void handleOTARoot() {
  // Serve the OTA HTML page from LittleFS
  if (LittleFS.exists("/ota.html")) {
    File file = LittleFS.open("/ota.html", "r");
    otaServer.streamFile(file, "text/html");
    file.close();
  } else {
    // Fallback if file not found
    String html = "<html><body>";
    html += "<h1>ESP32-S3 OTA Update</h1>";
    html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
    html += "<input type='file' name='update'>";
    html += "<input type='submit' value='Update'>";
    html += "</form></body></html>";
    otaServer.send(200, "text/html", html);
  }
}

void handleOTAUpdate() {
  // Called when the upload is completed
  if (Update.hasError()) {
    otaServer.send(200, "text/plain", "Update failed!");
  } else {
    otaServer.send(200, "text/plain", "Update successful! Rebooting...");
    delay(2000);
    ESP.restart();
  }
}

void handleOTADoUpdate() {
  // Called during file upload
  HTTPUpload& upload = otaServer.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    
    // Start LED blinking to indicate update in progress
    setLEDState(LED_OTA_MODE);
    
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    // Write the received bytes to the update
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
    
    // Print progress
    Serial.printf("Progress: %u bytes\n", upload.totalSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    // Finish the update
    if (Update.end(true)) {
      Serial.printf("Update Success: %u bytes\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void processOTAData(uint8_t* data, size_t len) {
  // Check if this is a control packet
  if (len >= 4 && data[0] == 'O' && data[1] == 'T' && data[2] == 'A') {
    // Control packet format: "OTA" + command
    char cmd = data[3];
    
    if (cmd == 'S') {  // Start update
      // Extract size from the rest of the packet (bytes 4-7)
      if (len >= 8) {
        otaBufferSize = ((uint32_t)data[4] << 24) | ((uint32_t)data[5] << 16) | 
                       ((uint32_t)data[6] << 8) | (uint32_t)data[7];
        
        Serial.printf("Starting OTA update of %u bytes\n", otaBufferSize);
        otaCurrentSize = 0;
        otaUpdateStarted = true;
        
        if (!Update.begin(otaBufferSize)) {
          Update.printError(Serial);
          otaUpdateStarted = false;
          
          // Send error response
          if (otaDeviceConnected && otaCharacteristic) {
            uint8_t response[4] = {'O', 'T', 'A', 'E'};  // Error
            otaCharacteristic->setValue(response, 4);
            otaCharacteristic->notify();
          }
        } else {
          // Send OK response
          if (otaDeviceConnected && otaCharacteristic) {
            uint8_t response[4] = {'O', 'T', 'A', 'K'};  // OK
            otaCharacteristic->setValue(response, 4);
            otaCharacteristic->notify();
          }
        }
      }
    } else if (cmd == 'E') {  // End update
      if (otaUpdateStarted) {
        if (Update.end(true)) {
          Serial.printf("OTA Update Success: %u bytes\n", otaCurrentSize);
          
          // Send success response
          if (otaDeviceConnected && otaCharacteristic) {
            uint8_t response[4] = {'O', 'T', 'A', 'S'};  // Success
            otaCharacteristic->setValue(response, 4);
            otaCharacteristic->notify();
          }
          
          // Reboot after successful update
          delay(2000);
          ESP.restart();
        } else {
          Update.printError(Serial);
          
          // Send error response
          if (otaDeviceConnected && otaCharacteristic) {
            uint8_t response[4] = {'O', 'T', 'A', 'E'};  // Error
            otaCharacteristic->setValue(response, 4);
            otaCharacteristic->notify();
          }
        }
        
        otaUpdateStarted = false;
      }
    }
  } else {
    // Data packet
    if (otaUpdateStarted) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
        
        // Send error response
        if (otaDeviceConnected && otaCharacteristic) {
          uint8_t response[4] = {'O', 'T', 'A', 'E'};  // Error
          otaCharacteristic->setValue(response, 4);
          otaCharacteristic->notify();
        }
        
        otaUpdateStarted = false;
      } else {
        otaCurrentSize += len;
        
        // Send progress response every ~10% or so
        if (otaCurrentSize % (otaBufferSize / 10) < len && otaDeviceConnected && otaCharacteristic) {
          uint8_t response[8] = {'O', 'T', 'A', 'P',  // Progress
                               (uint8_t)(otaCurrentSize >> 24),
                               (uint8_t)(otaCurrentSize >> 16),
                               (uint8_t)(otaCurrentSize >> 8),
                               (uint8_t)otaCurrentSize};
          otaCharacteristic->setValue(response, 8);
          otaCharacteristic->notify();
        }
      }
    }
  }
}
  ot