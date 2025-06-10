#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Config.h"

// WiFi credentials - update these with your network info
#define WIFI_SSID "Kosta wifi"
#define WIFI_PASSWORD "isiegay123"

// API endpoints - update with your server address
#define API_BASE_URL "https://health-monitoring-api-gaajasa6aac0b9dy.canadacentral-01.azurewebsites.net/api"
#define API_ENDPOINT API_BASE_URL "/SensorData"
#define REGISTER_ENDPOINT API_BASE_URL "/DeviceTokens/register"
#define GET_TOKEN_ENDPOINT API_BASE_URL "/get-test-token"
#define GET_DEVICE_CONFIG_ENDPOINT API_BASE_URL "/device-config/"
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
  bool fetchDeviceConfig();
  
private:
  bool isConnected;
  unsigned long lastDataSendTime;
  String authToken; 
  bool fetchAuthToken(); // New private method to get the token
  bool registerDeviceInternal(); // Renamed for clarity, called after token fetch
};

#endif // NETWORK_MANAGER_H
