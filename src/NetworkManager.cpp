#include "../include/NetworkManager.h"
#include <WiFiClientSecure.h>

NetworkManager::NetworkManager() {
  isConnected = false;
  lastDataSendTime = 0;
  authToken = ""; // Initialize authToken
}

// ---- NEW METHOD IMPLEMENTATION ----
bool NetworkManager::fetchAuthToken() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("FetchAuthToken: WiFi not connected.");
    return false;
  }

  // Switch to WiFiClientSecure
  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure(); // Insecure HTTPS (accepts all certificates)

  HTTPClient http;
  // Use client for HTTPS connection
  http.begin(*client, GET_TOKEN_ENDPOINT); 

  http.setTimeout(15000);
  http.setConnectTimeout(10000);
  
  Serial.print("Fetching auth token from: ");
  Serial.println(GET_TOKEN_ENDPOINT);
  
  int httpResponseCode = http.GET();
  Serial.print("HTTP Response Code: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode == 200) {
    authToken = http.getString();
    Serial.print("Auth token received: ");
    Serial.println(authToken);
    http.end();
    delete client;
    return (authToken.length() > 0);
  } else if (httpResponseCode > 0) {
    Serial.print("Server error. HTTP Code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.print("Response: ");
    Serial.println(response);
  } else {
    Serial.print("Connection error. HTTP Code: ");
    Serial.println(httpResponseCode);
    Serial.print("Error details: ");
    Serial.println(http.errorToString(httpResponseCode).c_str());
  }
  http.end();
  delete client;
  return false;
}
// ---- END NEW METHOD ----

bool NetworkManager::initialize() {
  Serial.println("Initializing WiFi connection...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    isConnected = true;

    Serial.println("Attempting to fetch authentication token...");
    if (fetchAuthToken()) {
      Serial.println("Auth token fetched successfully. Attempting to register device...");
      if (registerDeviceInternal()) {
        Serial.println("Device registered successfully.");
        return true;
      } else {
        Serial.println("Device registration failed after fetching token.");
        return false;
      }
    } else {
      Serial.println("Failed to fetch auth token. Cannot proceed with registration.");
      return false;
    }
  } else {
    Serial.println("\nWiFi connection failed!");
    isConnected = false; 
    return false;
  }
}

void NetworkManager::reconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi reconnected!");
      isConnected = true;
    } else {
      isConnected = false;
    }
  }
}

bool NetworkManager::sendSensorData(float accel, float gyro, bool fallDetected) {
  // Record time since last transmission for debugging
  unsigned long now = millis();
  unsigned long elapsed = now - lastDataSendTime;
  
  // Show transmission frequency info
  if (fallDetected) {
    Serial.println("FALL DETECTED - Sending emergency data immediately!");
  } else {
    Serial.print("Sending data - Time since last transmission: ");
    Serial.print(elapsed);
    Serial.println(" ms");
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    reconnect();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("SendSensorData: WiFi not connected.");
      return false;
    }
  }

  if (authToken.isEmpty()) {
    Serial.println("SendSensorData: Auth token is missing. Attempting to fetch...");
    if (!fetchAuthToken()) {
      Serial.println("SendSensorData: Failed to re-fetch auth token.");
      return false;
    }
  }

  // Create client and http objects
  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure();

  HTTPClient http;
  http.begin(*client, API_ENDPOINT);
  http.setTimeout(15000); // Increase timeout for Azure
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + authToken);

  // Build JSON payload

  Serial.print("Creating JSON with fallDetected = ");
  Serial.println(fallDetected ? "true" : "false");
  StaticJsonDocument<768> jsonDoc;
  jsonDoc["deviceId"] = DEVICE_ID;
  jsonDoc["accel"] = accel;
  jsonDoc["gyro"] = gyro;
  jsonDoc["fallDetected"] = fallDetected;
  jsonDoc["timestamp"] = now;  // Use the timestamp we captured at the start


  if(fallDetected) {
  jsonDoc["emergencyType"] = "FALL_DETECTED";
  jsonDoc["severity"] = "HIGH";
  jsonDoc["requiresResponse"] = true;
  jsonDoc["alertMessage"] = "Fall detected - immediate assistance may be required";

    JsonObject dataJson = jsonDoc.createNestedObject("dataJson");
  dataJson["accel"] = accel;
  dataJson["gyro"] = gyro;
  dataJson["deviceId"] = DEVICE_ID;
  dataJson["detectionTime"] = now;
  
  }
  String jsonString;
  serializeJson(jsonDoc, jsonString);


   Serial.print("JSON payload size: ");
  Serial.println(jsonString.length());
  Serial.print("JSON content: ");
  Serial.println(jsonString);
  // Send the data
  int httpResponseCode = http.POST(jsonString);

  if (httpResponseCode > 0) {
    String response = http.getString();
    if (fallDetected) {
      Serial.print("EMERGENCY DATA SENT! HTTP Response code: ");
    } else {
      Serial.print("Data sent. HTTP Response code: ");
    }
    Serial.println(httpResponseCode);
    
    Serial.println("Server Response: " + response);
    // Update last send time
    lastDataSendTime = now;
    http.end();
    delete client;
    return (httpResponseCode == 200 || httpResponseCode == 201);
  } else {
    Serial.println("Error on sending POST: " + String(httpResponseCode));
    Serial.print("sendSensorData: HTTPC error: ");
    Serial.println(http.errorToString(httpResponseCode).c_str());
    if (httpResponseCode == HTTP_CODE_UNAUTHORIZED || httpResponseCode == HTTP_CODE_FORBIDDEN) {
      authToken = "";
    }
    http.end();
    delete client;
    return false;
  }
}

bool NetworkManager::registerDeviceInternal() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("RegisterDeviceInternal: WiFi not connected.");
    return false; 
  }
  if (authToken.isEmpty()) {
    Serial.println("RegisterDeviceInternal: Auth token is missing.");
    return false;
  }

  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure();

  HTTPClient http;
  http.begin(*client, REGISTER_ENDPOINT);
  http.setTimeout(15000); // Increase timeout for Azure
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + authToken);

  StaticJsonDocument<768> jsonDoc;
  jsonDoc["deviceId"] = DEVICE_ID;
  jsonDoc["platform"] = "ESP32";
  jsonDoc["deviceName"] = "ESP32";
  jsonDoc["token"] = authToken;

  String jsonString;
  serializeJson(jsonDoc, jsonString);

  Serial.print("Registering device with payload: ");
  Serial.println(jsonString);

  int httpResponseCode = http.POST(jsonString);

  if (httpResponseCode > 0) {
    Serial.println("Device registration HTTP Response code: " + String(httpResponseCode));
    Serial.println("Response: " + http.getString());
    http.end();
    delete client;
    return (httpResponseCode == HTTP_CODE_OK || httpResponseCode == HTTP_CODE_CREATED);
  } else {
    Serial.println("Error on device registration: " + String(httpResponseCode));
    Serial.print("registerDeviceInternal: HTTPC error: ");
    Serial.println(http.errorToString(httpResponseCode).c_str());
    if (httpResponseCode == HTTP_CODE_UNAUTHORIZED || httpResponseCode == HTTP_CODE_FORBIDDEN) {
      authToken = "";
    }
    http.end();
    delete client;
    return false;
  }
}



bool NetworkManager::fetchDeviceConfig() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("FetchDeviceConfig: WiFi not connected.");
    return false;
  }

  if (authToken.isEmpty()) {
    Serial.println("FetchDeviceConfig: Auth token is missing. Attempting to fetch...");
    if (!fetchAuthToken()) {
      Serial.println("FetchDeviceConfig: Failed to fetch auth token.");
      return false;
    }
  }

  // Create secure client
  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure(); // Accept any certificate

  HTTPClient http;
  
  String configUrl = String(GET_DEVICE_CONFIG_ENDPOINT) + DEVICE_ID;
  Serial.print("Fetching device configuration from: ");
  Serial.println(configUrl);
  
  http.begin(*client, configUrl);
  http.addHeader("Authorization", "Bearer " + authToken);
  http.setTimeout(15000); // Increase timeout for Azure
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("Device configuration received:");
    Serial.println(response);
    
    // Parse configuration
    StaticJsonDocument<1024> configDoc;
    DeserializationError error = deserializeJson(configDoc, response);
    
    if (!error) {
      // Apply configuration settings
      // Example: float fallThreshold = configDoc["fallDetectionSensitivity"];
      http.end();
      delete client;
      return true;
    } else {
      Serial.print("JSON parsing error: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("Error fetching configuration. HTTP Code: ");
    Serial.println(httpResponseCode);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(response);
    }
  }
  
  http.end();
  delete client;
  return false;
}