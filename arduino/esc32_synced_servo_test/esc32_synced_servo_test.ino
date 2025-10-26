// ESP32 + 2x SG90 — Opposite direction motion (LEDC hardware PWM)

#include <Arduino.h>

/* ==== USER CONFIG ==== */
const int SERVO1_PIN = 5;    // GPIO for Servo #1 signal
const int SERVO2_PIN = 18;   // GPIO for Servo #2 signal

const int CH1 = 0;
const int CH2 = 1;

const int PWM_FREQ_HZ = 50;  // 50 Hz for RC servos
const int PWM_BITS   = 16;   // resolution

const int MIN_US = 500;      // ~0°
const int MID_US = 1500;     // ~90°
const int MAX_US = 2400;     // ~180°

uint32_t dutyMax;

/* ==== Helper Functions ==== */
inline uint32_t usToDuty(int us) {
  if (us < MIN_US) us = MIN_US;
  if (us > MAX_US) us = MAX_US;
  return (uint32_t)(((uint64_t)us * dutyMax) / 20000ULL); // 20ms period
}

inline void writeUS(int ch, int us) {
  ledcWrite(ch, usToDuty(us));
}

inline void writeAngle(int ch, int angleDeg, bool reverse = false) {
  // Clamp
  angleDeg = constrain(angleDeg, 0, 180);
  if (reverse) angleDeg = 180 - angleDeg;  // flip direction
  int us = map(angleDeg, 0, 180, MIN_US, MAX_US);
  writeUS(ch, us);
}

void attachServo(int pin, int ch) {
  ledcSetup(ch, PWM_FREQ_HZ, PWM_BITS);
  ledcAttachPin(pin, ch);
}

/* ==== Move Both (Opposite Directions) ==== */
void moveBothOpposite(int targetDeg, uint32_t durationMs) {
  static int curr = 90;
  targetDeg = constrain(targetDeg, 0, 180);

  const uint32_t stepMs = 10;
  uint32_t steps = max<uint32_t>(1, durationMs / stepMs);

  for (uint32_t i = 1; i <= steps; ++i) {
    int a = curr + (int)((targetDeg - curr) * (int)i / (int)steps);
    // Servo1 normal, Servo2 reversed
    writeAngle(CH1, a, false);
    writeAngle(CH2, a, true);
    delay(stepMs);
  }
  curr = targetDeg;
}

/* ==== Setup & Loop ==== */
void setup() {
  dutyMax = (1UL << PWM_BITS) - 1UL;
  attachServo(SERVO1_PIN, CH1);
  attachServo(SERVO2_PIN, CH2);

  // Center both
  writeUS(CH1, MID_US);
  writeUS(CH2, MID_US);
  delay(500);
}

void loop() {
  // Move both 90° apart, opposite directions
  moveBothOpposite(0, 800);   // Servo1 → 0°, Servo2 → 180°
  moveBothOpposite(110, 800); // Servo1 → 180°, Servo2 → 0°
}
