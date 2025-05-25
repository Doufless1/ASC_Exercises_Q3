#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Config.h"

// WiFi credentials - update these with your network info
#define WIFI_SSID "Homies101"
#define WIFI_PASSWORD "Onnoisgay123!"

// API endpoints - update with your server address
#define API_ENDPOINT "http://192.168.178.190:7012/api/SensorData"
#define REGISTER_ENDPOINT "http://192.168.178.190:7012/api/DeviceTokens/register"
#define GET_TOKEN_ENDPOINT "http://192.168.178.190:7012/api/get-test-token"

// Unique device identifier
#define DEVICE_ID "ESP32_FALL_001"

// Send data every 30 seconds
//#define SEND_INTERVAL_MS 30000

class NetworkManager {
public:
  NetworkManager();
  bool initialize(); // This will now also fetch the token and register
  bool sendSensorData(float accel, float gyro, bool fallDetected);
  void reconnect();
  
private:
  bool isConnected;
  unsigned long lastDataSendTime;
  String authToken; 
  bool fetchAuthToken(); // New private method to get the token
  bool registerDeviceInternal(); // Renamed for clarity, called after token fetch
};

#endif // NETWORK_MANAGER_H
