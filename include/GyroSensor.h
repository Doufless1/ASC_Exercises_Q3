#ifndef GYRO_SENSOR_H
#define GYRO_SENSOR_H

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <CircularBuffer.h>
#include "Config.h"

class GyroSensor {
public:
  GyroSensor();
  
  bool initialize();
  void calibrate();
  void process();
  void getAccelGyroData(float &accelMagnitude, float &gyroMagnitude);
  
  float getAccelX();
  float getAccelY();
  float getAccelZ();
  
  float getGyroX();
  float getGyroY();
  float getGyroZ();
  
  CircularBuffer<float, ACCEL_BUFFER_SIZE>& getAccelBuffer();
  CircularBuffer<float, GYRO_BUFFER_SIZE>& getGyroBuffer();
  
private:
  // we use the CircularBuffer because it helps with RAM which we have limited of because lets say the buffer is full its going to remove old data and replace it with a new one and its easier to find states that happend like falling emrgencty etc. jsut in general more benefitial and as u can see we have two instance of him the private one is the acual buffer. the second one with the & they give reference to the accual buffer without having to copy them everytime they are used which would limit our memory even more.
  Adafruit_MPU6050 mpu;
  CircularBuffer<float, ACCEL_BUFFER_SIZE> accelBuffer;
  CircularBuffer<float, GYRO_BUFFER_SIZE> gyroBuffer;
  
  float accelCalibX, accelCalibY, accelCalibZ;
  float gyroCalibX, gyroCalibY, gyroCalibZ;
  
  float lastAccelX, lastAccelY, lastAccelZ;
  float lastGyroX, lastGyroY, lastGyroZ;

float lastAccelMagnitude;
  float lastGyroMagnitude;
};

#endif // GYRO_SENSOR_H
