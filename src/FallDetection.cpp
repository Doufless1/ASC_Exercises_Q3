#include "../include/FallDetection.h"

FallDetection::FallDetection(GyroSensor &sensor) : gyroSensor(sensor) {
  currentState = STATE_INIT;
  fallDetected = false;
  fallTimestamp = 0;
}

bool FallDetection::detectFall() {
  // Step 1: Check for free-fall conditions
  // Free-fall is characterized by near-zero acceleration
  // Accelerometers don't measure absolute movement - they measure the force acting on a small mass inside the sensor. When falling:
//  `Gravity is pulling everything (both the sensor and its internal test mass) at the same rate
//  The test mass inside the accelerometer isn't pressing against any surface
//  With no differential force, the accelerometer reads close to zero
  bool freeFallDetected = false;
  auto &accelBuffer = gyroSensor.getAccelBuffer();
  
  for (int i = 0; i < accelBuffer.size(); i++) {
    if (accelBuffer[i] < FREEFALL_THRESHOLD) {
      freeFallDetected = true;
      break;
    }
  }
  
  if (!freeFallDetected) {
    return false;
  }
  
  // Step 2: Look for impact after free-fall
  unsigned long impactCheckStart = millis();
  bool impactDetected = false;
  
  // Monitor for impact for up to 1000ms after detecting free-fall
  while (millis() - impactCheckStart < 1000 && !impactDetected) {
    float accelMagnitude, gyroMagnitude;
    gyroSensor.process();
    gyroSensor.getAccelGyroData(accelMagnitude, gyroMagnitude);
    
    if (accelMagnitude > IMPACT_THRESHOLD) {
      impactDetected = true;
      Serial.printf("Impact detected: %.2f m/s²\n", accelMagnitude);
      break;
    }
    
    delay(5);
  }
  
  return impactDetected;
}

bool FallDetection::detectInactivityAfterImpact() {
  Serial.println("Monitoring post-fall movement patterns...");
  
  unsigned long startTime = millis();
  int consecutiveStillSamples = 0;
  
  // Check for a period of stillness (medical emergency sign)
  while (millis() - startTime < POST_IMPACT_WINDOW_MS) {
    float accelMagnitude, gyroMagnitude;
    gyroSensor.process();
    gyroSensor.getAccelGyroData(accelMagnitude, gyroMagnitude);
    
    // Calculate dynamic acceleration (removing gravity component)
    float dynamicAccel = abs(accelMagnitude - 9.8); // Remove gravity magnitude
    
    if (dynamicAccel < INACTIVITY_THRESHOLD) {
      consecutiveStillSamples++;
    } else {
      consecutiveStillSamples = 0; // Reset counter if movement detected
    }
    
    // If we observe enough consecutive still samples, it's likely a medical emergency
    if (consecutiveStillSamples >= REQUIRED_STILL_SAMPLES) {
      Serial.println("Prolonged inactivity detected - possible medical emergency");
      return true;
    }
    
    delay(10);
  }
  
  // If we exit the loop without detecting enough stillness, likely not a medical emergency
  return false;
}

void FallDetection::triggerAlarm() {
  Serial.println("MEDICAL EMERGENCY ALARM ACTIVATED!");
  
  
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
}

void FallDetection::cancelAlarm() {
  digitalWrite(LED_PIN, LOW);
  fallDetected = false;
  
  Serial.println("Alarm canceled");
}

SystemState FallDetection::getState() {
  return currentState;
}

void FallDetection::setState(SystemState state) {
  currentState = state;
}
