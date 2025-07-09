#include "cellular.h"
#include "hw_config.h"
#include "config.h"
#include <SoftwareSerial.h>

SoftwareSerial simSerial(SIM7000_RX_PIN, SIM7000_TX_PIN);
bool cellularInitialized = false;
bool cellularConnected = false;

bool initCellular() {
  // Initialize UART for SIM7000G
  simSerial.begin(115200);
  
  // Power on sequence for SIM7000G
  pinMode(SIM7000_PWR_PIN, OUTPUT);
  pinMode(SIM7000_RST_PIN, OUTPUT);
  
  // Reset module
  digitalWrite(SIM7000_RST_PIN, LOW);
  delay(100);
  digitalWrite(SIM7000_RST_PIN, HIGH);
  delay(2000);
  
  // Power on module if needed (pulse power key)
  digitalWrite(SIM7000_PWR_PIN, HIGH);
  delay(1500);
  digitalWrite(SIM7000_PWR_PIN, LOW);
  delay(5000);  // Wait for module to initialize
  
  // Check if module is responsive
  String response = sendATCommand("AT");
  if (response.indexOf("OK") == -1) {
    Serial.println("No response from SIM7000G module");
    return false;
  }
  
  // Configure module
  sendATCommand("AT+CMEE=2");  // Enable verbose error messages
  
  // Set module to function mode
  sendATCommand("AT+CFUN=1");
  
  // Configure APN for data connection
  sendATCommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  sendATCommand("AT+SAPBR=3,1,\"APN\",\"" + String(APN_NAME) + "\"");
  
  cellularInitialized = true;
  Serial.println("SIM7000G module initialized successfully");
  return true;
}

bool connectCellular() {
  if (!cellularInitialized) {
    if (!initCellular()) {
      return false;
    }
  }
  
  // Check if SIM card is ready
  String response = sendATCommand("AT+CPIN?");
  if (response.indexOf("READY") == -1) {
    Serial.println("SIM card not ready");
    return false;
  }
  
  // Wait for network registration
  int retry = 0;
  bool registered = false;
  while (retry < CELLULAR_RETRY_COUNT && !registered) {
    response = sendATCommand("AT+CREG?");
    if (response.indexOf("+CREG: 0,1") != -1 || response.indexOf("+CREG: 0,5") != -1) {
      registered = true;
    } else {
      Serial.println("Waiting for network registration...");
      delay(2000);
      retry++;
    }
  }
  
  if (!registered) {
    Serial.println("Failed to register to network");
    return false;
  }
  
  // Check signal quality
  response = sendATCommand("AT+CSQ");
  Serial.println("Signal quality: " + response);
  
  // Open GPRS context
  retry = 0;
  bool contextOpened = false;
  while (retry < CELLULAR_RETRY_COUNT && !contextOpened) {
    response = sendATCommand("AT+SAPBR=1,1", 10000);
    if (response.indexOf("OK") != -1) {
      contextOpened = true;
    } else {
      Serial.println("Retrying to open GPRS context...");
      delay(1000);
      retry++;
    }
  }
  
  if (!contextOpened) {
    Serial.println("Failed to open GPRS context");
    return false;
  }
  
  // Check if context is active
  response = sendATCommand("AT+SAPBR=2,1");
  if (response.indexOf("+SAPBR: 1,1") == -1) {
    Serial.println("GPRS context not active");
    return false;
  }
  
  cellularConnected = true;
  Serial.println("Cellular connection established");
  return true;
}

void disconnectCellular() {
  if (!cellularConnected) {
    return;
  }
  
  // Close GPRS context
  sendATCommand("AT+SAPBR=0,1");
  
  cellularConnected = false;
  Serial.println("Cellular connection closed");
}

bool isCellularConnected() {
  return cellularConnected;
}

String sendATCommand(const String& command, unsigned long timeout) {
  // Clear any pending data
  while (simSerial.available()) {
    simSerial.read();
  }
  
  // Send command
  simSerial.println(command);
  Serial.print("AT> " + command + "\n");
  
  // Collect response
  String response = "";
  unsigned long startTime = millis();
  
  while (millis() - startTime < timeout) {
    if (simSerial.available()) {
      char c = simSerial.read();
      response += c;
      
      // Check if response is complete (ends with OK or ERROR)
      if (response.endsWith("\r\nOK\r\n") || response.endsWith("\r\nERROR\r\n")) {
        break;
      }
    }
    yield();  // Allow other tasks to run
  }
  
  Serial.println("AT< " + response);
  return response;
}

bool connectTCP(const String& host, int port) {
  if (!cellularConnected && !connectCellular()) {
    return false;
  }
  
  // Close any existing connection
  sendATCommand("AT+CIPSHUT", 5000);
  
  // Configure TCP/IP parameters
  sendATCommand("AT+CIPMUX=0");  // Single connection mode
  
  // Start TCP connection
  String cmd = "AT+CIPSTART=\"TCP\",\"" + host + "\"," + String(port);
  String response = sendATCommand(cmd, 20000);
  
  return (response.indexOf("CONNECT OK") != -1 || response.indexOf("ALREADY CONNECT") != -1);
}

bool disconnectTCP() {
  // Close TCP connection
  String response = sendATCommand("AT+CIPCLOSE", 5000);
  sendATCommand("AT+CIPSHUT", 5000);
  
  return (response.indexOf("CLOSE OK") != -1);
}

bool sendTCPData(const String& data) {
  // Send data command
  String cmd = "AT+CIPSEND=" + String(data.length());
  String response = sendATCommand(cmd, 5000);
  
  if (response.indexOf(">") == -1) {
    return false;
  }
  
  // Send the actual data
  simSerial.print(data);
  delay(500);
  
  // Wait for SEND OK
  response = "";
  unsigned long startTime = millis();
  
  while (millis() - startTime < 10000) {
    if (simSerial.available()) {
      char c = simSerial.read();
      response += c;
      
      if (response.indexOf("SEND OK") != -1) {
        Serial.println("TCP data sent successfully");
        return true;
      }
      
      if (response.indexOf("SEND FAIL") != -1 || response.indexOf("ERROR") != -1) {
        Serial.println("Failed to send TCP data");
        return false;
      }
    }
    yield();
  }
  
  Serial.println("TCP send timeout");
  return false;
}

String receiveTCPData(int timeout) {
  String data = "";
  unsigned long startTime = millis();
  
  while (millis() - startTime < timeout) {
    if (simSerial.available()) {
      char c = simSerial.read();
      data += c;
      
      // Check for the pattern: +CIPRCV: len,data
      if (data.indexOf("+CIPRCV:") != -1 && data.endsWith("\r\n")) {
        int dataStart = data.indexOf(",");
        if (dataStart != -1) {
          data = data.substring(dataStart + 1);
          data.trim();
          return data;
        }
      }
    }
    yield();
  }
  
  return data;
}

bool httpGet(const String& url, String& response) {
  if (!cellularConnected && !connectCellular()) {
    return false;
  }
  
  // Initialize HTTP service
  sendATCommand("AT+HTTPINIT");
  sendATCommand("AT+HTTPPARA=\"CID\",1");
  
  // Set URL
  sendATCommand("AT+HTTPPARA=\"URL\",\"" + url + "\"");
  
  // Execute GET request
  String result = sendATCommand("AT+HTTPACTION=0", 30000);
  
  // Check for success (200 OK)
  if (result.indexOf("+HTTPACTION: 0,200") == -1) {
    sendATCommand("AT+HTTPTERM");
    return false;
  }
  
  // Read response
  result = sendATCommand("AT+HTTPREAD", 10000);
  
  // Extract response data
  int responseStartIndex = result.indexOf("\r\n") + 2;
  if (responseStartIndex >= 2) {
    response = result.substring(responseStartIndex);
  }
  
  // Terminate HTTP service
  sendATCommand("AT+HTTPTERM");
  
  return true;
}

bool httpPost(const String& url, const String& contentType, const String& data, String& response) {
  if (!cellularConnected && !connectCellular()) {
    return false;
  }
  
  // Initialize HTTP service
  sendATCommand("AT+HTTPINIT");
  sendATCommand("AT+HTTPPARA=\"CID\",1");
  
  // Set URL
  sendATCommand("AT+HTTPPARA=\"URL\",\"" + url + "\"");
  
  // Set content type
  sendATCommand("AT+HTTPPARA=\"CONTENT\",\"" + contentType + "\"");
  
  // Prepare to send data
  String cmd = "AT+HTTPDATA=" + String(data.length()) + ",10000";
  String result = sendATCommand(cmd);
  
  if (result.indexOf("DOWNLOAD") == -1) {
    sendATCommand("AT+HTTPTERM");
    return false;
  }
  
  // Send data
  simSerial.print(data);
  delay(500);
  
  // Execute POST request
  result = sendATCommand("AT+HTTPACTION=1", 30000);
  
  // Check for success (200 OK)
  if (result.indexOf("+HTTPACTION: 1,200") == -1) {
    sendATCommand("AT+HTTPTERM");
    return false;
  }
  
  // Read response
  result = sendATCommand("AT+HTTPREAD", 10000);
  
  // Extract response data
  int responseStartIndex = result.indexOf("\r\n") + 2;
  if (responseStartIndex >= 2) {
    response = result.substring(responseStartIndex);
  }
  
  // Terminate HTTP service
  sendATCommand("AT+HTTPTERM");
  
  return true;
}
