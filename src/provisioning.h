#ifndef PROVISIONING_H
#define PROVISIONING_H

#include <Arduino.h>

// Start provisioning (WiFi/BLE)
bool startProvisioning();

// Stop provisioning
void stopProvisioning();

// Process provisioning events (call in loop)
void handleProvisioning();

// Check if provisioning is active
bool isProvisioningActive();

// Process JSON data from provisioning
void handleProvisioningJSON(String jsonData);

// Mark device as provisioned
void markAsProvisioned();

#endif // PROVISIONING_H
