#include "../include/FallDetection.h"

FallDetection::FallDetection(GyroSensor &sensor) : gyroSensor(sensor) {
  currentState = STATE_INIT;
  fallDetected = false;
  fallTimestamp = 0;
}

bool FallDetection::detectFall() {
  static bool inFreeFall = false;
  static unsigned long freeFallStartTime = 0;
  static unsigned long lastDebugTime = 0;
  static float minAccel = 100.0;
  static float maxAccel = 0.0;
  static unsigned long lastResetTime = 0;
  float accelMagnitude, gyroMagnitude;  
  
  
  gyroSensor.getAccelGyroData(accelMagnitude, gyroMagnitude);

  
  // reset min/max values every 3 seconds
  if (millis() - lastResetTime > 3000) {
    minAccel = 100.0;
    maxAccel = 0.0;
    lastResetTime = millis();
  }
  
  // track min/max values for debugging
  if (accelMagnitude < minAccel) minAccel = accelMagnitude;
  if (accelMagnitude > maxAccel) maxAccel = accelMagnitude;
  
  // debug output
  if (millis() - lastDebugTime > 500) {
    lastDebugTime = millis();
    Serial.printf("Fall Detection Debug | Current: %.2f | Min: %.2f | Max: %.2f | FF Threshold: %.2f | Impact Threshold: %.2f | State: %s\n", 
                 accelMagnitude, 
                 minAccel, 
                 maxAccel, 
                 FREEFALL_THRESHOLD, 
                 IMPACT_THRESHOLD,
                 inFreeFall ? "IN FREE-FALL" : "normal");
  }
  
  // If not in free-fall, check for free-fall condition
  if (!inFreeFall) {
    if (accelMagnitude < FREEFALL_THRESHOLD) {
      inFreeFall = true;
      freeFallStartTime = millis();
      Serial.printf("\n!!! FREE-FALL DETECTED !!! Acceleration: %.2f m/s²\n", accelMagnitude);
      digitalWrite(LED_PIN, HIGH);
      delay(50);
      digitalWrite(LED_PIN, LOW);
    }
  }
  // If in free-fall, check for impact or timeout
  else {
    if (accelMagnitude > IMPACT_THRESHOLD) {
      Serial.printf("\n!!! IMPACT DETECTED !!! Acceleration: %.2f m/s²\n", accelMagnitude);
      Serial.println("FALL SEQUENCE COMPLETE - DETECTED BOTH FREE-FALL AND IMPACT");
      inFreeFall = false; // Reset for next detection
      return true;
    }
    
    // check for timeout (free-fall window expired)
    if (millis() - freeFallStartTime > 1000) {
      Serial.println("Free-fall timeout - no impact detected within time window");
      inFreeFall = false;
    }
  }
  
  return false;
}

bool FallDetection::detectInactivityAfterImpact() {
  Serial.println("Monitoring post-fall movement patterns...");
  
  unsigned long startTime = millis();
  int consecutiveStillSamples = 0;
  int totalSamples = 0;
  
  // check for a period of stillness (medical emergency sign)
  while (millis() - startTime < POST_IMPACT_WINDOW_MS) {
    float accelMagnitude, gyroMagnitude;
    gyroSensor.process();
    gyroSensor.getAccelGyroData(accelMagnitude, gyroMagnitude);
    totalSamples++;
    
    // calculate dynamic acceleration (removing gravity component)
    float dynamicAccel = abs(accelMagnitude - 9.8); // Remove gravity magnitude
    
    if (dynamicAccel < INACTIVITY_THRESHOLD) {
      consecutiveStillSamples++;
      if (consecutiveStillSamples % 10 == 0) {
        Serial.printf("Still samples: %d/%d\n", consecutiveStillSamples, REQUIRED_STILL_SAMPLES);
      }
    } else {
      Serial.printf("Movement detected: %.2f (threshold: %.2f)\n", dynamicAccel, INACTIVITY_THRESHOLD);
      consecutiveStillSamples = 0; 
    }
    
    
    if (consecutiveStillSamples >= REQUIRED_STILL_SAMPLES) {
      Serial.printf("EMERGENCY CONFIRMED: %d consecutive still samples detected\n", consecutiveStillSamples);
      return true;
    }
    
    delay(10);
  }
  
  Serial.printf("Inactivity check complete - movement detected (%d still samples, needed %d)\n", 
               consecutiveStillSamples, REQUIRED_STILL_SAMPLES);
  return false;
}

void FallDetection::triggerAlarm() {
  Serial.println("\n!!! MEDICAL EMERGENCY ALARM ACTIVATED !!!");
  Serial.println("Turn on RED LED and alert caregivers");
  
  // Quick visual feedback with rapid blinks
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  
  
  digitalWrite(LED_PIN, HIGH);
}

void FallDetection::cancelAlarm() {
  digitalWrite(LED_PIN, LOW);
  fallDetected = false;
  
  Serial.println("Alarm canceled - returning to normal monitoring");
}

SystemState FallDetection::getState() {
  return currentState;
}

void FallDetection::setState(SystemState state) {
  static const char* stateNames[] = {
    "INIT", "CALIBRATING", "MONITORING", "FALL_DETECTED", "ALARM_ACTIVE"
  };
  
  Serial.printf("State changing: %s -> %s\n", 
               stateNames[currentState], stateNames[state]);
  
  currentState = state;
}
