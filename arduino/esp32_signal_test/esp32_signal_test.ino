#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// Pin definitions
const int SERVO_PIN = 18;
const int TRIG_PIN  = 26;
const int ECHO_PIN  = 25;

// Servo angles
const int OPEN  = 30;
const int CLOSED = 100;

const int TRIGGER_DISTANCE = 10;  // cm

Servo servo;
bool proximityEnabled = false;  // Proximity sensor only active when locked

const char* ssid = "engsci2t8pey";
const char* password = "gooncave123";

WebServer server(80);

const int SERVO_SPEED = 10;  // milliseconds between each degree

void moveServoSlowly(int targetAngle) {
    Serial.print("Moving from ");
    Serial.print(servo.read());
    Serial.print(" to ");
    Serial.println(targetAngle);
    
    int currentAngle = servo.read();
    int step = (targetAngle > currentAngle) ? 1 : -1;
    
    while (currentAngle != targetAngle) {
        currentAngle += step;
        servo.write(currentAngle);
        delay(SERVO_SPEED);
    }
    
    Serial.println("Movement complete");
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

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32 Starting ===");
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  servo.attach(SERVO_PIN);
  servo.write(OPEN);
  Serial.println("Servo initialized to OPEN");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  server.on("/command", HTTP_POST, handleCommand);
  server.begin();
  Serial.println("HTTP server started on port 80");
}

void loop() {
  server.handleClient();
  
  // Only check proximity sensor when locked
  if (proximityEnabled) {
    int distance = getDistance();
    
    // Only print distance occasionally to reduce serial spam
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 1000) {
      Serial.print("Distance: ");
      Serial.print(distance);
      Serial.println(" cm");
      lastPrint = millis();
    }
    
    // Close door when object detected
    if (distance < TRIGGER_DISTANCE) {
      Serial.println("Object detected! Closing door...");
      moveServoSlowly(CLOSED);
      proximityEnabled = false;  // Disable until next lock command
    }
  }
  
  delay(50);
}

void handleCommand() {
  Serial.println("=== Command received ===");
  
  if (server.method() != HTTP_POST) { 
    Serial.println("ERROR: Not POST");
    server.send(405); 
    return; 
  }
  
  String body = server.arg("plain");
  Serial.print("Body: ");
  Serial.println(body);
  
  StaticJsonDocument<200> doc;
  DeserializationError err = deserializeJson(doc, body);
  
  if (err) { 
    Serial.println("ERROR: Bad JSON");
    server.send(400, "application/json", "{\"error\":\"bad json\"}"); 
    return; 
  }
  
  const char* cmd = doc["cmd"];
  Serial.print("Command: ");
  Serial.println(cmd);

  // Send response IMMEDIATELY before moving servo
  server.send(200, "application/json", "{\"status\":\"ok\"}");
  Serial.println("Response sent");

  // Now handle command
  if (String(cmd) == "cmd_locked") {
    Serial.println("LOCKED: Enabling proximity sensor");
    proximityEnabled = true;
    // Don't move servo yet - wait for proximity trigger
  }
  else if (String(cmd) == "cmd_unlocked") {
    Serial.println("UNLOCKED: Disabling proximity sensor and opening door");
    proximityEnabled = false;
    moveServoSlowly(OPEN);
  }
  else {
    Serial.print("Unknown command: ");
    Serial.println(cmd);
  }
}
