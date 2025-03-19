#ifndef FALL_DETECTION_H
#define FALL_DETECTION_H

#include "Config.h"
#include "GyroSensor.h"

class FallDetection {
public:
  FallDetection(GyroSensor &sensor);
  
  bool detectFall();
  bool detectInactivityAfterImpact();
  void triggerAlarm();
  void cancelAlarm();
  
  SystemState getState();
  void setState(SystemState state);
  
private:
  GyroSensor &gyroSensor;
  SystemState currentState;
  unsigned long fallTimestamp;
  bool fallDetected;
};

#endif // FALL_DETECTION_H
