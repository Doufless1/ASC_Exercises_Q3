#include <Arduino.h>
#include "../include/Config.h"
#include "../include/Button.h"
#include "../include/GyroSensor.h"
#include "../include/FallDetection.h"


GyroSensor gyroSensor;
Button button;
FallDetection *fallDetection = NULL;


unsigned long lastSampleTime = 0;
unsigned long lastDebugOutput = 0;
unsigned long fallTimestamp = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  

  
  pinMode(LED_PIN, OUTPUT);
  
  // visual startup sequence - triple blink
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
  
  Serial.println("Initializing MPU6050 sensor...");
  if (!gyroSensor.initialize()) {
    Serial.println("Failed to initialize MPU6050!");
    // error indicator - rapid led blinking
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
  
  
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  if (button.isPressed()) {
    Serial.println("Button pressed");
    
    // cancel aarm if in alarm state
    if (fallDetection->getState() == STATE_FALL_DETECTED || 
        fallDetection->getState() == STATE_ALARM_ACTIVE) {
      fallDetection->cancelAlarm();
      fallDetection->setState(STATE_MONITORING);
      fallTimestamp = 0;
    }
    
    button.clearPressFlag();
  }
  
  // main state machine
  switch (fallDetection->getState()) {
    case STATE_MONITORING:
      // sample sensor at regular intervals
      if (millis() - lastSampleTime >= SAMPLING_PERIOD_MS) {
        lastSampleTime = millis();
        gyroSensor.process();
        
        // detect falls
        if (fallDetection->detectFall()) {
          Serial.println("POTENTIAL FALL DETECTED - Monitoring for inactivity");
          digitalWrite(LED_PIN, HIGH); 
          fallTimestamp = millis();
          fallDetection->setState(STATE_FALL_DETECTED);
        }
      }
      
      // debug output every second
      /*
       *Stay responsive: The device must constantly monitor for falls and button presses
Process data in real-time: We can't afford to pause for debug output
Manage multiple timing requirements: We have different intervals for sampling, debouncing, etc
        */
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
      // check if alarm delay has passed
      if (fallTimestamp > 0 && millis() - fallTimestamp >= ALARM_DELAY_MS) {
        // check for post-fall inactivity (medical emergency)
        if (fallDetection->detectInactivityAfterImpact()) {
          fallDetection->triggerAlarm();
          fallDetection->setState(STATE_ALARM_ACTIVE);
        } else {
          // false alarm, return to monitoring
          Serial.println("Movement detected after fall - likely not an emergency");
          fallDetection->cancelAlarm();
          fallDetection->setState(STATE_MONITORING);
          fallTimestamp = 0;
        }
      }
      
      // led blink pattern for fall detection
      digitalWrite(LED_PIN, (millis() % 1000) < 500);
      break;
      
    case STATE_ALARM_ACTIVE:
      // visual alarm - LED rapid blinking
      digitalWrite(LED_PIN, (millis() % 300) < 150);
      break;
      
    default:
      break;
  }
}
