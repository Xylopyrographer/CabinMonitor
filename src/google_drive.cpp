#include "google_drive.h"
#include "storage.h"
#include "cellular.h"
#include "led_control.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <GDrive.h>

// Credentials storage
#define CREDENTIALS_FILE "/google_creds.json"

// Google Drive API parameters
GDrive gDrive;
bool driveInitialized = false;

// Google Drive credentials
String client_id;
String client_secret;
String refresh_token;
String folder_id;

bool isGoogleDriveConfigured() {
  if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS");
    return false;
  }
  
  if (!LittleFS.exists(CREDENTIALS_FILE)) {
    Serial.println("Google Drive credentials not found");
    return false;
  }
  
  File file = LittleFS.open(CREDENTIALS_FILE, "r");
  if (!file) {
    Serial.println("Failed to open credentials file");
    return false;
  }
  
  // Check if file is empty or malformed
  if (file.size() == 0) {
    file.close();
    Serial.println("Credentials file is empty");
    return false;
  }
  
  // Parse JSON credentials
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.println("Failed to parse credentials JSON");
    return false;
  }
  
  // Check if all required fields are present
  if (!doc.containsKey("client_id") || 
      !doc.containsKey("client_secret") || 
      !doc.containsKey("refresh_token") || 
      !doc.containsKey("folder_id")) {
    Serial.println("Missing required credential fields");
    return false;
  }
  
  // Everything looks valid
  return true;
}

bool initGoogleDrive() {
  if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS");
    return false;
  }
  
  if (!LittleFS.exists(CREDENTIALS_FILE)) {
    Serial.println("Google Drive credentials not found");
    return false;
  }
  
  File file = LittleFS.open(CREDENTIALS_FILE, "r");
  if (!file) {
    Serial.println("Failed to open credentials file");
    return false;
  }
  
  // Parse JSON credentials
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.println("Failed to parse credentials JSON");
    return false;
  }
  
  // Extract credentials
  client_id = doc["client_id"].as<String>();
  client_secret = doc["client_secret"].as<String>();
  refresh_token = doc["refresh_token"].as<String>();
  folder_id = doc["folder_id"].as<String>();
  
  // Initialize Google Drive client
  gDrive.begin(client_id.c_str(), client_secret.c_str(), refresh_token.c_str());
  
  driveInitialized = true;
  Serial.println("Google Drive initialized successfully");
  return true;
}

bool uploadFilesToGoogleDrive() {
  if (!driveInitialized && !initGoogleDrive()) {
    Serial.println("Google Drive not initialized");
    return false;
  }
  
  if (!isCellularConnected() && !connectCellular()) {
    Serial.println("Failed to connect cellular for upload");
    return false;
  }
  
  // Get file count
  int fileCount = getFileCount();
  if (fileCount == 0) {
    Serial.println("No files to upload");
    return true;  // Not an error
  }
  
  Serial.printf("Found %d files to upload\n", fileCount);
  
  // Iterate through files and upload each one
  bool allSuccess = true;
  
  for (int i = 0; i < fileCount; i++) {
    String filename = getFileName(i);
    if (filename.length() == 0) {
      continue;  // Skip if filename is empty
    }
    
    Serial.printf("Uploading file %d/%d: %s\n", i+1, fileCount, filename.c_str());
    
    // Update LED to show upload activity
    setLEDState(LED_UPLOADING);
    
    if (!uploadFileToGoogleDrive(filename)) {
      Serial.printf("Failed to upload file: %s\n", filename.c_str());
      allSuccess = false;
    } else {
      Serial.printf("Successfully uploaded: %s\n", filename.c_str());
    }
  }
  
  return allSuccess;
}

bool uploadFileToGoogleDrive(const String& filename) {
  if (!driveInitialized && !initGoogleDrive()) {
    return false;
  }
  
  if (!isCellularConnected() && !connectCellular()) {
    return false;
  }
  
  if (!SD.exists(filename)) {
    Serial.printf("File not found: %s\n", filename.c_str());
    return false;
  }
  
  // Extract just the filename part (without path)
  String basename = filename;
  if (basename.startsWith("/")) {
    basename = basename.substring(1);
  }
  
  // Open the file
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.printf("Failed to open file: %s\n", filename.c_str());
    return false;
  }
  
  // Get file size
  size_t fileSize = file.size();
  if (fileSize == 0) {
    Serial.println("File is empty");
    file.close();
    return false;
  }
  
  // Set up upload parameters
  bool result = gDrive.uploadFile(basename.c_str(), "image/jpeg", folder_id.c_str(), 
                                 [&file](uint8_t *buffer, size_t bufferSize) -> size_t {
                                   return file.read(buffer, bufferSize);
                                 },
                                 fileSize);
  
  file.close();
  
  return result;
}

bool saveGoogleDriveCredentials(const String& clientId, const String& clientSecret, 
                               const String& refreshToken, const String& folderId) {
  if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS");
    return false;
  }
  
  // Create JSON document with credentials
  StaticJsonDocument<512> doc;
  doc["client_id"] = clientId;
  doc["client_secret"] = clientSecret;
  doc["refresh_token"] = refreshToken;
  doc["folder_id"] = folderId;
  
  // Open file for writing
  File file = LittleFS.open(CREDENTIALS_FILE, "w");
  if (!file) {
    Serial.println("Failed to open credentials file for writing");
    return false;
  }
  
  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write credentials to file");
    file.close();
    return false;
  }
  
  file.close();
  
  // Update the global variables
  client_id = clientId;
  client_secret = clientSecret;
  refresh_token = refreshToken;
  folder_id = folderId;
  
  Serial.println("Google Drive credentials saved successfully");
  return true;
}

String getGoogleDriveFolderId() {
  return folder_id;
}
