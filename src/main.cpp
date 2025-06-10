#include <Arduino.h>
#include "../include/Config.h"
#include "../include/Button.h"
#include "../include/GyroSensor.h"
#include "../include/FallDetection.h"
#include "../include/NetworkManager.h"  // Add this line

GyroSensor gyroSensor;
Button button;
FallDetection *fallDetection = NULL;
NetworkManager networkManager;  // Add this line

unsigned long lastSampleTime = 0;
unsigned long lastDebugOutput = 0;
unsigned long fallTimestamp = 0;
bool fallReported = false;  // Track if fall has been reported to server

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(LED_PIN, OUTPUT);
  
  // Visual startup sequence - triple blink
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
  
  Serial.println("Initializing MPU6050 sensor...");
  if (!gyroSensor.initialize()) {
    Serial.println("Failed to initialize MPU6050!");
    // Error indicator - rapid LED blinking
    while (1) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  }
  
  button.initialize();
  
  fallDetection = new FallDetection(gyroSensor);
  
  Serial.println("Calibrating sensor - keep device still...");
  fallDetection->setState(STATE_CALIBRATING);
  digitalWrite(LED_PIN, HIGH); 
  
  gyroSensor.calibrate();
  
  digitalWrite(LED_PIN, LOW); 
  fallDetection->setState(STATE_MONITORING);
  Serial.println("System ready and monitoring for falls");
  
  // Initialize WiFi, fetch token, and register device
  // The initialize() method now handles token fetching and internal registration
  if (!networkManager.initialize()) { // This line now does everything needed
    Serial.println("Network initialization failed! Check WiFi and server.");
    // You might want to add an error indicator here too, like LED blinking
  }


   if (networkManager.fetchDeviceConfig()) {
      Serial.println("Device configuration loaded successfully");
    } else {
      Serial.println("Using default configuration values");
    }
  // ---- REMOVE THE LINE BELOW ----
  // networkManager.registerDeviceInternal(); // THIS LINE WAS CAUSING THE ERROR
  // ---- END REMOVAL ----
  
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  if (button.isPressed()) {
    Serial.println("Button pressed");
    
    // Cancel alarm if in alarm state
    if (fallDetection->getState() == STATE_FALL_DETECTED || 
        fallDetection->getState() == STATE_ALARM_ACTIVE) {
      fallDetection->cancelAlarm();
      fallDetection->setState(STATE_MONITORING);
      fallTimestamp = 0;
      fallReported = false;  // Reset fall reported flag
    }
    
    button.clearPressFlag();
  }
  
  // Main state machine
  switch (fallDetection->getState()) {
    case STATE_MONITORING:
      // Sample sensor at regular intervals
      if (millis() - lastSampleTime >= SAMPLING_PERIOD_MS) {
        lastSampleTime = millis();
        gyroSensor.process();
        
        // Detect falls
        if (fallDetection->detectFall()) {
          Serial.println("POTENTIAL FALL DETECTED - Monitoring for inactivity");
          digitalWrite(LED_PIN, HIGH); 
          fallTimestamp = millis();
          fallDetection->setState(STATE_FALL_DETECTED);
        }
        
        // Send regular sensor updates to server
        float accelMagnitude, gyroMagnitude;
        gyroSensor.getAccelGyroData(accelMagnitude, gyroMagnitude);
        networkManager.sendSensorData(accelMagnitude, gyroMagnitude, false);
      }
      
      // Debug output every second
      if (millis() - lastDebugOutput >= 1000) {
        lastDebugOutput = millis();
        float accelMagnitude, gyroMagnitude;
        gyroSensor.getAccelGyroData(accelMagnitude, gyroMagnitude);
        
        Serial.print("Accel: ");
        Serial.print(accelMagnitude);
        Serial.print(" m/s²  |  Gyro: ");
        Serial.print(gyroMagnitude);
        Serial.println(" deg/s");
      }
      break;
      
    case STATE_FALL_DETECTED:
      // Check if alarm delay has passed
      if (fallTimestamp > 0 && millis() - fallTimestamp >= ALARM_DELAY_MS) {
        // Check for post-fall inactivity (medical emergency)
        if (fallDetection->detectInactivityAfterImpact()) {
          fallDetection->triggerAlarm();
          fallDetection->setState(STATE_ALARM_ACTIVE);
          
          // Send fall alert to server immediately
          if (!fallReported) {
            float accelMagnitude, gyroMagnitude;
            gyroSensor.getAccelGyroData(accelMagnitude, gyroMagnitude);
            networkManager.sendSensorData(accelMagnitude, gyroMagnitude, true);
            fallReported = true;  // Mark as reported
          }
        } else {
          // False alarm, return to monitoring
          Serial.println("Movement detected after fall - likely not an emergency");
          fallDetection->cancelAlarm();
          fallDetection->setState(STATE_MONITORING);
          fallTimestamp = 0;
        }
      }
      
      // LED blink pattern for fall detection
      digitalWrite(LED_PIN, (millis() % 1000) < 500);
      break;
      
    case STATE_ALARM_ACTIVE:
      // Visual alarm - LED rapid blinking
      digitalWrite(LED_PIN, (millis() % 300) < 150);
      break;
      
    default:
      break;
  }
}
