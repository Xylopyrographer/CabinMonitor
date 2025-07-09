#ifndef CELLULAR_H
#define CELLULAR_H

#include <Arduino.h>

// Initialize the SIM7000G cellular module
bool initCellular();

// Connect to the cellular network
bool connectCellular();

// Disconnect from the cellular network
void disconnectCellular();

// Check if cellular is connected
bool isCellularConnected();

// Send an AT command and return the response
String sendATCommand(const String& command, unsigned long timeout = 3000);

// TCP/IP functions
bool connectTCP(const String& host, int port);
bool disconnectTCP();
bool sendTCPData(const String& data);
String receiveTCPData(int timeout = 10000);

// HTTP functions
bool httpGet(const String& url, String& response);
bool httpPost(const String& url, const String& contentType, const String& data, String& response);

#endif // CELLULAR_H
