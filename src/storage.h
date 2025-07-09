#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

// Initialization
bool initStorage();

// File operations
bool savePhotoToSD(const char* filename, const uint8_t* data, size_t len);
bool fileExists(const char* filename);
void listAllFiles();
void deleteAllFiles();
size_t getFreeSpaceSD();

// File listing for upload
int getFileCount();
String getFileName(int index);

// Time utilities
void getTimestampString(char* buffer, size_t bufferSize);

#endif // STORAGE_H
