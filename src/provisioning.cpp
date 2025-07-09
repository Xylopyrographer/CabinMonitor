#include "provisioning.h"
#include "led_control.h"
#include "config.h"
#include "google_drive.h"
#include "sms_messaging.h"
#include "storage.h"
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// WiFi AP Mode
WebServer server(80);
bool provisioningActive = false;
bool wifiProvisioningActive = false;

// BLE Mode
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool bleProvisioningActive = false;
bool deviceConnected = false;

// Provisioning settings
int lightThreshold = LIGHT_THRESHOLD_IR_ENABLE;
int soundThreshold = SOUND_DETECTION_THRESHOLD;
String baseFileName = BASE_FILENAME;
String phoneNumber = "";
String activityMsg = "Activity detected";
String noActivityMsg = "No activity detected";
String noCommMsg = "No communication to Google Drive";
String commOkMsg = "Communication to Google Drive OK";
String disabledMsg = "Monitoring disabled";
String enabledMsg = "Monitoring enabled";

class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* server) {
    deviceConnected = true;
    Serial.println("BLE Client connected");
  }
  
  void onDisconnect(BLEServer* server) {
    deviceConnected = false;
    Serial.println("BLE Client disconnected");
    // Restart advertising
    BLEDevice::startAdvertising();
  }
};

class CharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.println("Received BLE data:");
      
      // Parse the received JSON data
      String jsonData = String(value.c_str());
      handleProvisioningJSON(jsonData);
    }
  }
};

// Forward declarations of functions
void setupWiFiAP();
void setupWebServer();
void setupBLE();
void handleRoot();
void handleSetCredentials();
void handleSetSettings();
void handleProvisioningJSON(String jsonData);
void markAsProvisioned();

bool startProvisioning() {
  if (provisioningActive) {
    return true;  // Already active
  }
  
  // Initialize SPIFFS for web files
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return false;
  }
  
  // Set LED to indicate provisioning mode
  setLEDState(LED_PROVISIONING);
  
  // Start both WiFi AP and BLE
  setupWiFiAP();
  setupBLE();
  
  provisioningActive = true;
  Serial.println("Provisioning mode started");
  return true;
}

void stopProvisioning() {
  if (!provisioningActive) {
    return;
  }
  
  // Stop WiFi AP if active
  if (wifiProvisioningActive) {
    server.stop();
    WiFi.softAPdisconnect(true);
    wifiProvisioningActive = false;
    Serial.println("WiFi AP stopped");
  }
  
  // Stop BLE if active
  if (bleProvisioningActive) {
    if (pServer) {
      pServer->getAdvertising()->stop();
      bleProvisioningActive = false;
      Serial.println("BLE advertising stopped");
    }
  }
  
  provisioningActive = false;
  Serial.println("Provisioning mode stopped");
}

void handleProvisioning() {
  if (!provisioningActive) {
    return;
  }
  
  // Handle WiFi server requests
  if (wifiProvisioningActive) {
    server.handleClient();
  }
  
  // For BLE, we use callbacks so no explicit handling needed here
}

bool isProvisioningActive() {
  return provisioningActive;
}

void setupWiFiAP() {
  // Set up WiFi in AP mode
  WiFi.mode(WIFI_AP);
  
  String ssid = String(DEVICE_NAME) + "-" + String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFF), HEX);
  
  if (WiFi.softAP(ssid.c_str(), WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONNECTIONS)) {
    Serial.println("WiFi AP started: " + ssid);
    Serial.println("IP address: " + WiFi.softAPIP().toString());
    
    // Start web server
    setupWebServer();
    wifiProvisioningActive = true;
  } else {
    Serial.println("Failed to start WiFi AP");
  }
}

void setupWebServer() {
  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/setcredentials", HTTP_POST, handleSetCredentials);
  server.on("/setsettings", HTTP_POST, handleSetSettings);
  
  // Start server
  server.begin();
  Serial.println("Web server started");
}

void setupBLE() {
  // Initialize BLE
  BLEDevice::init(String(DEVICE_NAME).c_str());
  
  // Create BLE server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  // Create BLE service
  BLEService *pService = pServer->createService(BLE_SERVICE_UUID);
  
  // Create BLE characteristic
  pCharacteristic = pService->createCharacteristic(
                      BLE_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  
  pCharacteristic->setCallbacks(new CharacteristicCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  
  // Start the service
  pService->start();
  
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE advertising started");
  bleProvisioningActive = true;
}

void handleRoot() {
  // Serve the HTML page from SPIFFS
  if (SPIFFS.exists("/index.html")) {
    File file = SPIFFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  } else {
    // Fallback if file not found
    String html = "<html><body>";
    html += "<h1>ESP32-S3 Monitoring Device Setup</h1>";
    
    // Google Drive credentials form
    html += "<h2>Google Drive Credentials</h2>";
    html += "<form action='/setcredentials' method='post'>";
    html += "Client ID: <input type='text' name='client_id'><br>";
    html += "Client Secret: <input type='text' name='client_secret'><br>";
    html += "Refresh Token: <input type='text' name='refresh_token'><br>";
    html += "Folder ID: <input type='text' name='folder_id'><br>";
    html += "<input type='submit' value='Save Credentials'>";
    html += "</form>";
    
    // Device settings form
    html += "<h2>Device Settings</h2>";
    html += "<form action='/setsettings' method='post'>";
    html += "Light Threshold (0-100): <input type='number' name='light_threshold' min='0' max='100' value='" + String(lightThreshold) + "'><br>";
    html += "Sound Level Threshold: <input type='number' name='sound_threshold' value='" + String(soundThreshold) + "'><br>";
    html += "Base Filename (2-4 chars): <input type='text' name='base_filename' minlength='2' maxlength='4' value='" + baseFileName + "'><br>";
    html += "Phone Number (max 13 digits): <input type='tel' name='phone_number' maxlength='13' value='" + phoneNumber + "'><br>";
    html += "Activity Detected Message: <input type='text' name='activity_msg' maxlength='80' value='" + activityMsg + "'><br>";
    html += "No Activity Message: <input type='text' name='no_activity_msg' maxlength='80' value='" + noActivityMsg + "'><br>";
    html += "No Communication Message: <input type='text' name='no_comm_msg' maxlength='80' value='" + noCommMsg + "'><br>";
    html += "Communication OK Message: <input type='text' name='comm_ok_msg' maxlength='80' value='" + commOkMsg + "'><br>";
    html += "Monitoring Disabled Message: <input type='text' name='disabled_msg' maxlength='80' value='" + disabledMsg + "'><br>";
    html += "Monitoring Enabled Message: <input type='text' name='enabled_msg' maxlength='80' value='" + enabledMsg + "'><br>";
    html += "<input type='submit' value='Save Settings'>";
    html += "</form>";
    
    html += "</body></html>";
    server.send(200, "text/html", html);
  }
}

void handleSetCredentials() {
  String client_id = server.arg("client_id");
  String client_secret = server.arg("client_secret");
  String refresh_token = server.arg("refresh_token");
  String folder_id = server.arg("folder_id");
  
  if (client_id.length() == 0 || client_secret.length() == 0 || 
      refresh_token.length() == 0 || folder_id.length() == 0) {
    server.send(400, "text/plain", "Missing required fields");
    return;
  }
  
  if (saveGoogleDriveCredentials(client_id, client_secret, refresh_token, folder_id)) {
    server.send(200, "text/plain", "Credentials saved successfully");
    markAsProvisioned();
  } else {
    server.send(500, "text/plain", "Failed to save credentials");
  }
}

void handleSetSettings() {
  // Retrieve form values
  String lightThresholdStr = server.arg("light_threshold");
  String soundThresholdStr = server.arg("sound_threshold");
  String baseFileNameVal = server.arg("base_filename");
  String phoneNumberVal = server.arg("phone_number");
  String activityMsgVal = server.arg("activity_msg");
  String noActivityMsgVal = server.arg("no_activity_msg");
  String noCommMsgVal = server.arg("no_comm_msg");
  String commOkMsgVal = server.arg("comm_ok_msg");
  String disabledMsgVal = server.arg("disabled_msg");
  String enabledMsgVal = server.arg("enabled_msg");
  
  bool success = true;
  String errorMsg = "";
  
  // Validate and save light threshold
  if (lightThresholdStr.length() > 0) {
    int lightVal = lightThresholdStr.toInt();
    if (lightVal >= 0 && lightVal <= 100) {
      Preferences preferences;
      if (preferences.begin("sensors", false)) {
        preferences.putInt("light_thr", lightVal);
        preferences.end();
        lightThreshold = lightVal;
      } else {
        success = false;
        errorMsg += "Failed to save light threshold. ";
      }
    } else {
      success = false;
      errorMsg += "Light threshold must be between 0 and 100. ";
    }
  }
  
  // Validate and save sound threshold
  if (soundThresholdStr.length() > 0) {
    int soundVal = soundThresholdStr.toInt();
    if (soundVal > 0) {
      Preferences preferences;
      if (preferences.begin("sensors", false)) {
        preferences.putInt("sound_thr", soundVal);
        preferences.end();
        soundThreshold = soundVal;
      } else {
        success = false;
        errorMsg += "Failed to save sound threshold. ";
      }
    } else {
      success = false;
      errorMsg += "Sound threshold must be greater than 0. ";
    }
  }
  
  // Validate and save base filename
  if (baseFileNameVal.length() >= 2 && baseFileNameVal.length() <= 4) {
    if (setBaseFilename(baseFileNameVal)) {
      baseFileName = baseFileNameVal;
    } else {
      success = false;
      errorMsg += "Failed to save base filename. ";
    }
  } else if (baseFileNameVal.length() > 0) {
    success = false;
    errorMsg += "Base filename must be 2-4 characters. ";
  }
  
  // Validate and save SMS settings
  if (phoneNumberVal.length() > 0 || 
      activityMsgVal.length() > 0 || 
      noActivityMsgVal.length() > 0 || 
      noCommMsgVal.length() > 0 || 
      commOkMsgVal.length() > 0 || 
      disabledMsgVal.length() > 0 || 
      enabledMsgVal.length() > 0) {
    
    // Default values if not provided
    if (phoneNumberVal.length() == 0) phoneNumberVal = phoneNumber;
    if (activityMsgVal.length() == 0) activityMsgVal = activityMsg;
    if (noActivityMsgVal.length() == 0) noActivityMsgVal = noActivityMsg;
    if (noCommMsgVal.length() == 0) noCommMsgVal = noCommMsg;
    if (commOkMsgVal.length() == 0) commOkMsgVal = commOkMsg;
    if (disabledMsgVal.length() == 0) disabledMsgVal = disabledMsg;
    if (enabledMsgVal.length() == 0) enabledMsgVal = enabledMsg;
    
    if (saveSMSSettings(
          phoneNumberVal, 
          activityMsgVal, 
          noActivityMsgVal, 
          noCommMsgVal, 
          commOkMsgVal, 
          disabledMsgVal, 
          enabledMsgVal)) {
      
      phoneNumber = phoneNumberVal;
      activityMsg = activityMsgVal;
      noActivityMsg = noActivityMsgVal;
      noCommMsg = noCommMsgVal;
      commOkMsg = commOkMsgVal;
      disabledMsg = disabledMsgVal;
      enabledMsg = enabledMsgVal;
    } else {
      success = false;
      errorMsg += "Failed to save SMS settings. Check phone number format and message lengths. ";
    }
  }
  
  if (success) {
    server.send(200, "text/plain", "Settings saved successfully");
    markAsProvisioned();
  } else {
    server.send(400, "text/plain", "Error saving settings: " + errorMsg);
  }
}

void handleProvisioningJSON(String jsonData) {
  // Parse the JSON data
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    
    // Send error response through BLE if connected
    if (deviceConnected && pCharacteristic) {
      String response = "{\"status\":\"error\",\"message\":\"Invalid JSON format\"}";
      pCharacteristic->setValue(response.c_str());
      pCharacteristic->notify();
    }
    return;
  }
  
  // Determine if this is credentials or settings
  bool isCredentials = doc.containsKey("client_id") && doc.containsKey("client_secret");
  bool isSettings = doc.containsKey("light_threshold") || doc.containsKey("sound_threshold") || 
                    doc.containsKey("base_filename") || doc.containsKey("phone_number");
  
  bool success = true;
  String errorMsg = "";
  
  // Process Google Drive credentials if present
  if (isCredentials) {
    String client_id = doc["client_id"] | "";
    String client_secret = doc["client_secret"] | "";
    String refresh_token = doc["refresh_token"] | "";
    String folder_id = doc["folder_id"] | "";
    
    // Validate credentials
    if (client_id.length() == 0 || client_secret.length() == 0 || 
        refresh_token.length() == 0 || folder_id.length() == 0) {
      errorMsg += "Missing required Google Drive credential fields. ";
      success = false;
    } else {
      // Save credentials
      if (!saveGoogleDriveCredentials(client_id, client_secret, refresh_token, folder_id)) {
        errorMsg += "Failed to save Google Drive credentials. ";
        success = false;
      }
    }
  }
  
  // Process device settings if present
  if (isSettings) {
    // Light threshold
    if (doc.containsKey("light_threshold")) {
      int lightVal = doc["light_threshold"];
      if (lightVal >= 0 && lightVal <= 100) {
        Preferences preferences;
        if (preferences.begin("sensors", false)) {
          preferences.putInt("light_thr", lightVal);
          preferences.end();
          lightThreshold = lightVal;
        } else {
          success = false;
          errorMsg += "Failed to save light threshold. ";
        }
      } else {
        success = false;
        errorMsg += "Light threshold must be between 0 and 100. ";
      }
    }
    
    // Sound threshold
    if (doc.containsKey("sound_threshold")) {
      int soundVal = doc["sound_threshold"];
      if (soundVal > 0) {
        Preferences preferences;
        if (preferences.begin("sensors", false)) {
          preferences.putInt("sound_thr", soundVal);
          preferences.end();
          soundThreshold = soundVal;
        } else {
          success = false;
          errorMsg += "Failed to save sound threshold. ";
        }
      } else {
        success = false;
        errorMsg += "Sound threshold must be greater than 0. ";
      }
    }
    
    // Base filename
    if (doc.containsKey("base_filename")) {
      String baseFileNameVal = doc["base_filename"];
      if (baseFileNameVal.length() >= 2 && baseFileNameVal.length() <= 4) {
        if (setBaseFilename(baseFileNameVal)) {
          baseFileName = baseFileNameVal;
        } else {
          success = false;
          errorMsg += "Failed to save base filename. ";
        }
      } else {
        success = false;
        errorMsg += "Base filename must be 2-4 characters. ";
      }
    }
    
    // SMS settings
    String phoneNumberVal = doc["phone_number"] | phoneNumber;
    String activityMsgVal = doc["activity_msg"] | activityMsg;
    String noActivityMsgVal = doc["no_activity_msg"] | noActivityMsg;
    String noCommMsgVal = doc["no_comm_msg"] | noCommMsg;
    String commOkMsgVal = doc["comm_ok_msg"] | commOkMsg;
    String disabledMsgVal = doc["disabled_msg"] | disabledMsg;
    String enabledMsgVal = doc["enabled_msg"] | enabledMsg;
    
    if (doc.containsKey("phone_number") || doc.containsKey("activity_msg") || 
        doc.containsKey("no_activity_msg") || doc.containsKey("no_comm_msg") || 
        doc.containsKey("comm_ok_msg") || doc.containsKey("disabled_msg") || 
        doc.containsKey("enabled_msg")) {
      
      if (!saveSMSSettings(
            phoneNumberVal, 
            activityMsgVal, 
            noActivityMsgVal, 
            noCommMsgVal, 
            commOkMsgVal, 
            disabledMsgVal, 
            enabledMsgVal)) {
        success = false;
        errorMsg += "Failed to save SMS settings. ";
      } else {
        phoneNumber = phoneNumberVal;
        activityMsg = activityMsgVal;
        noActivityMsg = noActivityMsgVal;
        noCommMsg = noCommMsgVal;
        commOkMsg = commOkMsgVal;
        disabledMsg = disabledMsgVal;
        enabledMsg = enabledMsgVal;
      }
    }
  }
  
  // Send response via BLE
  if (deviceConnected && pCharacteristic) {
    String response;
    if (success) {
      response = "{\"status\":\"success\",\"message\":\"Settings saved successfully\"}";
      markAsProvisioned();
    } else {
      response = "{\"status\":\"error\",\"message\":\"" + errorMsg + "\"}";
    }
    pCharacteristic->setValue(response.c_str());
    pCharacteristic->notify();
  }
  
  // If success and both credentials and settings are provided, we can stop provisioning
  if (success && isCredentials) {
    // But wait a bit to allow the BLE client to receive the response
    delay(1000);
    stopProvisioning();
  }
}

void markAsProvisioned() {
  Preferences preferences;
  if (preferences.begin("monitoring", false)) {
    preferences.putBool("provisioned", true);
    preferences.end();
    Serial.println("Device marked as provisioned");
  }
}
