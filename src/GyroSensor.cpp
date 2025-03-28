#include "../include/GyroSensor.h"

GyroSensor::GyroSensor() {
  accelCalibX = accelCalibY = accelCalibZ = 0;
  gyroCalibX = gyroCalibY = gyroCalibZ = 0;
  
  lastAccelX = lastAccelY = lastAccelZ = 0;
  lastGyroX = lastGyroY = lastGyroZ = 0;
}

bool GyroSensor::initialize() {
  
  Wire.begin(SDA_PIN, SCL_PIN);
  
  
  if (!mpu.begin(0x68, &Wire)) {
    Serial.println("Failed to find MPU6050 at address 0x68, trying 0x69...");
    
    if (!mpu.begin(0x69, &Wire)) {
      Serial.println("Could not find a valid MPU6050 sensor!");
      return false;
    } else {
      Serial.println("MPU6050 found at address 0x69");
    }
  } else {
    Serial.println("MPU6050 found at address 0x68");
  }
  
  // edit the sensor settings
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  delay(100); 
  return true;
}

void GyroSensor::calibrate() {
  accelCalibX = accelCalibY = accelCalibZ = 0;
  gyroCalibX = gyroCalibY = gyroCalibZ = 0;
  int calibrationSamples = 0;
  
  unsigned long startTime = millis();
  const int totalSamples = 100;
  
  
  while (calibrationSamples < totalSamples && millis() - startTime < 5000) {
    sensors_event_t a, g, temp;
    if (mpu.getEvent(&a, &g, &temp)) {
      accelCalibX += a.acceleration.x;
      accelCalibY += a.acceleration.y;
      accelCalibZ += a.acceleration.z - 9.8; // remove gravity component from Z
      
      gyroCalibX += g.gyro.x;
      gyroCalibY += g.gyro.y;
      gyroCalibZ += g.gyro.z;
      
      calibrationSamples++;
      
      
      if (calibrationSamples % 10 == 0) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      }
      delay(20);
    }
  }
  
  // Calculate average offsets
  /*
   *
   * MEMS (Micro-Electro-Mechanical Systems) sensors like the MPU6050 have inherent manufacturing imperfections that cause them to report slightly inaccurate values even when perfectly still. 
   * So we use the offest to take the avrage error values
   */
  if (calibrationSamples > 0) {
    accelCalibX /= calibrationSamples;
    accelCalibY /= calibrationSamples;
    accelCalibZ /= calibrationSamples;
    gyroCalibX /= calibrationSamples;
    gyroCalibY /= calibrationSamples;
    gyroCalibZ /= calibrationSamples;
  }
  
  Serial.println("Calibration complete!");
  Serial.printf("Accel offsets: X: %.3f, Y: %.3f, Z: %.3f\n", 
                accelCalibX, accelCalibY, accelCalibZ);
  Serial.printf("Gyro offsets: X: %.3f, Y: %.3f, Z: %.3f\n",
                gyroCalibX, gyroCalibY, gyroCalibZ);
}

void GyroSensor::process() {
  sensors_event_t a, g, temp;
  
  if (mpu.getEvent(&a, &g, &temp)) {
    // store raw data with calibration offsets applied
    lastAccelX = a.acceleration.x - accelCalibX;
    lastAccelY = a.acceleration.y - accelCalibY;
    lastAccelZ = a.acceleration.z - accelCalibZ;
    
    lastGyroX = g.gyro.x - gyroCalibX;
    lastGyroY = g.gyro.y - gyroCalibY;
    lastGyroZ = g.gyro.z - gyroCalibZ;
    
   
  // This is calculating the total acceleration magnitude using the 3D Pythagorean theorem. Since accelerometers measure along three separate axes (X, Y, Z), we need to combine them to get the overall acceleration
    lastAccelMagnitude = sqrt(lastAccelX*lastAccelX + lastAccelY*lastAccelY + lastAccelZ*lastAccelZ);

  // This calculates the total rotational velocity magnitude, again by combining all three axes, and then converts it to degrees per second
    lastGyroMagnitude = sqrt(lastGyroX*lastGyroX + lastGyroY*lastGyroY + lastGyroZ*lastGyroZ) * RAD_TO_DEG;
    
    // store in circular buffers
    accelBuffer.push(lastAccelMagnitude);
    gyroBuffer.push(lastGyroMagnitude);
  }
}

void GyroSensor::getAccelGyroData(float &accelMagnitude, float &gyroMagnitude) {

  accelMagnitude = lastAccelMagnitude;
  gyroMagnitude = lastGyroMagnitude;
}

float GyroSensor::getAccelX() { return lastAccelX; }
float GyroSensor::getAccelY() { return lastAccelY; }
float GyroSensor::getAccelZ() { return lastAccelZ; }

float GyroSensor::getGyroX() { return lastGyroX; }
float GyroSensor::getGyroY() { return lastGyroY; }
float GyroSensor::getGyroZ() { return lastGyroZ; }

CircularBuffer<float, ACCEL_BUFFER_SIZE>& GyroSensor::getAccelBuffer() {
  return accelBuffer;
}

CircularBuffer<float, GYRO_BUFFER_SIZE>& GyroSensor::getGyroBuffer() {
  return gyroBuffer;
}
