#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>


#define SDA_PIN           21
#define SCL_PIN           22
#define LED_PIN           2    // Onboard LED
#define BUTTON_PIN        35   // Button pin (was GPIO0)


#define FREEFALL_THRESHOLD        0.4F   // g-force threshold for free-fall detection
#define IMPACT_THRESHOLD          2.5F   // g-force threshold for impact detection
#define INACTIVITY_THRESHOLD      0.2F   // g-force threshold for post-fall inactivity
#define GYRO_THRESHOLD            250.0F // degrees/s threshold for significant rotation
#define PRE_IMPACT_WINDOW_MS      500    // Size of pre-impact buffer window
#define POST_IMPACT_WINDOW_MS     2000   // Time to monitor after impact
#define ALARM_DELAY_MS            10000  // Delay before full alarm after fall detection
#define SAMPLING_PERIOD_MS        10     // Sample sensor every 10ms
#define REQUIRED_STILL_SAMPLES    50     // ~500ms of stillness to confirm emergency


#define ACCEL_BUFFER_SIZE         (PRE_IMPACT_WINDOW_MS / SAMPLING_PERIOD_MS)
#define GYRO_BUFFER_SIZE          (PRE_IMPACT_WINDOW_MS / SAMPLING_PERIOD_MS)

// PRE_IMPACT_WINDOW_MS: This defines how much history (in milliseconds) we want to keep before an impact. For example, 1000ms would mean we want to analyze 1 second of motion data leading up to a potential fall
// SAMPLING_PERIOD_MS: This is how often we take sensor readings. For example, 20ms would mean we sample at 50Hz (50 times per second). Mhz
// and we divide to keep it memory efficient for the exact value of sampelding examples we are going to need.
enum SystemState {
  STATE_INIT,
  STATE_CALIBRATING,
  STATE_MONITORING,
  STATE_FALL_DETECTED,
  STATE_ALARM_ACTIVE
};

#endif // CONFIG_H
