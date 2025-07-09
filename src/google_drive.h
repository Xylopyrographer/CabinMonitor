#ifndef GOOGLE_DRIVE_H
#define GOOGLE_DRIVE_H

#include <Arduino.h>

// Check if Google Drive credentials are configured
bool isGoogleDriveConfigured();

// Initialize Google Drive API
bool initGoogleDrive();

// Upload files to Google Drive
bool uploadFilesToGoogleDrive();

// Upload a specific file to Google Drive
bool uploadFileToGoogleDrive(const String& filename);

// Save Google Drive credentials to flash storage
bool saveGoogleDriveCredentials(const String& clientId, const String& clientSecret, const String& refreshToken, const String& folderId);

// Get Google Drive folder ID
String getGoogleDriveFolderId();

#endif // GOOGLE_DRIVE_H
