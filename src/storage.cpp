#include "storage.h"
#include "config.h"
#include <LittleFS.h>
#include <SD.h>

// Global variables
String baseName = BASE_FILENAME;
bool sdCardInitialized = false;
bool fsInitialized = false;

// Initialize storage (SD card and LittleFS)
bool initStorage() {
  // Initialize LittleFS for config storage
  if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS");
    fsInitialized = false;
  } else {
    fsInitialized = true;
    Serial.println("LittleFS mounted successfully");
  }
  
  // Initialize SD card
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
  
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Failed to mount SD card");
    sdCardInitialized = false;
    return false; // SD card is critical for operation
  }
  
  sdCardInitialized = true;
  Serial.println("SD card mounted successfully");
  
  // Check SD card space
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  uint64_t usedSpace = getUsedSpace() / (1024 * 1024);
  uint64_t freeSpace = cardSize - usedSpace;
  
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  Serial.printf("Used: %lluMB\n", usedSpace);
  Serial.printf("Free: %lluMB\n", freeSpace);
  
  if (freeSpace < MIN_SD_FREE_SPACE_MB) {
    Serial.println("Warning: Low SD card space");
  }
  
  // Check for unsent files
  int fileCount = getFileCount();
  if (fileCount > 0) {
    Serial.printf("Found %d unsent files on SD card\n", fileCount);
  }
  
  return true;
}

// Save photo to SD card
bool savePhotoToSD(const String& filename, uint8_t* data, size_t length) {
  if (!sdCardInitialized) {
    Serial.println("SD card not initialized");
    return false;
  }
  
  // Check if file already exists
  if (SD.exists(filename)) {
    Serial.printf("File %s already exists, deleting\n", filename.c_str());
    SD.remove(filename);
  }
  
  // Create new file
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.printf("Failed to create file: %s\n", filename.c_str());
    return false;
  }
  
  // Write data to file
  size_t bytesWritten = file.write(data, length);
  file.close();
  
  if (bytesWritten != length) {
    Serial.println("Failed to write complete file");
    return false;
  }
  
  Serial.printf("Photo saved to SD card: %s (%u bytes)\n", filename.c_str(), length);
  return true;
}

// Delete file from SD card
bool deleteFile(const String& filename) {
  if (!sdCardInitialized) {
    return false;
  }
  
  if (!SD.exists(filename)) {
    Serial.printf("File not found: %s\n", filename.c_str());
    return false;
  }
  
  if (SD.remove(filename)) {
    Serial.printf("File deleted: %s\n", filename.c_str());
    return true;
  } else {
    Serial.printf("Failed to delete file: %s\n", filename.c_str());
    return false;
  }
}

// Delete all files from SD card
bool deleteAllFiles() {
  if (!sdCardInitialized) {
    return false;
  }
  
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory");
    return false;
  }
  
  if (!root.isDirectory()) {
    Serial.println("Root is not a directory");
    root.close();
    return false;
  }
  
  bool success = true;
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String filename = "/" + String(file.name());
      file.close();
      
      if (!SD.remove(filename)) {
        Serial.printf("Failed to delete file: %s\n", filename.c_str());
        success = false;
      } else {
        Serial.printf("Deleted file: %s\n", filename.c_str());
      }
    }
    file = root.openNextFile();
  }
  
  root.close();
  return success;
}

// Get number of files on SD card
int getFileCount() {
  if (!sdCardInitialized) {
    return 0;
  }
  
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory");
    return 0;
  }
  
  if (!root.isDirectory()) {
    Serial.println("Root is not a directory");
    root.close();
    return 0;
  }
  
  int count = 0;
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      count++;
    }
    file.close();
    file = root.openNextFile();
  }
  
  root.close();
  return count;
}

// Get filename by index
String getFileName(int index) {
  if (!sdCardInitialized || index < 0) {
    return "";
  }
  
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory");
    return "";
  }
  
  if (!root.isDirectory()) {
    Serial.println("Root is not a directory");
    root.close();
    return "";
  }
  
  int count = 0;
  String filename = "";
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      if (count == index) {
        filename = "/" + String(file.name());
        file.close();
        break;
      }
      count++;
    }
    file.close();
    file = root.openNextFile();
  }
  
  root.close();
  return filename;
}

// Get used space on SD card
uint64_t getUsedSpace() {
  if (!sdCardInitialized) {
    return 0;
  }
  
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory");
    return 0;
  }
  
  if (!root.isDirectory()) {
    Serial.println("Root is not a directory");
    root.close();
    return 0;
  }
  
  uint64_t usedSpace = 0;
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      usedSpace += file.size();
    }
    file.close();
    file = root.openNextFile();
  }
  
  root.close();
  return usedSpace;
}

// Set base filename
bool setBaseFilename(const String& name) {
  // Check if name is valid (2-4 characters)
  if (name.length() < 2 || name.length() > 4) {
    Serial.println("Base filename must be 2-4 characters");
    return false;
  }
  
  // Save to preferences
  Preferences preferences;
  if (preferences.begin("storage", false)) {
    preferences.putString("basename", name);
    preferences.end();
    
    baseName = name;
    Serial.printf("Base filename set to: %s\n", name.c_str());
    return true;
  }
  
  return false;
}

// Get base filename
String getBaseFilename() {
  // Try to load from preferences first
  Preferences preferences;
  if (preferences.begin("storage", true)) {
    String name = preferences.getString("basename", BASE_FILENAME);
    preferences.end();
    
    // Update global variable
    baseName = name;
    return name;
  }
  
  return baseName;
}
