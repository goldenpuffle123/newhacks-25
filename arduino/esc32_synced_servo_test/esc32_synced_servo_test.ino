#include <ESP32Servo.h>

// Pin definitions
const int SERVO_PIN = 18;
const int TRIG_PIN  = 26;
const int ECHO_PIN  = 25;

// Servo angles
const int OPEN  = 20;
const int CLOSED = 70;

// Trigger settings
const int TRIGGER_DISTANCE = 10;  // cm
const int CLOSE_TIME = 5000;       // 5 seconds

Servo servo;

void setup() {
  Serial.begin(115200);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  servo.attach(SERVO_PIN);
  servo.write(OPEN);  // Start open
}

void loop() {
  int distance = getDistance();
  Serial.println(distance);
  
  if (distance < TRIGGER_DISTANCE) {
    servo.write(CLOSED);
    delay(CLOSE_TIME);
    servo.write(OPEN);
  }
  
  delay(100);
}

int getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999;
  
  return duration / 58;  // Convert to cm
}
