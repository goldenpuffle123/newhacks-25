#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// Pin definitions
const int SERVO_PIN = 18;
const int TRIG_PIN  = 26;
const int ECHO_PIN  = 25;

// Servo angles
const int OPEN  = 27;
const int CLOSED = 57;

const int TRIGGER_DISTANCE = 10;  // cm

Servo servo;

const char* ssid = "engsci2t8pey";
const char* password = "gooncave123";

WebServer server(80);

const int SERVO_SPEED = 20;  // milliseconds between each degree

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
        delay(SERVO_SPEED);  // Changed from 500 to 20ms
    }
    
    Serial.println("Movement complete");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  servo.attach(SERVO_PIN);
  servo.write(OPEN);

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
  delay(10);  // Small delay to prevent watchdog issues
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

  // Now move servo (won't block the HTTP response)
  if (String(cmd) == "cmd_locked") {
    Serial.println("Executing LOCK");
    moveServoSlowly(CLOSED);
  }
  else if (String(cmd) == "cmd_unlocked") {
    Serial.println("Executing UNLOCK");
    moveServoSlowly(OPEN);
  }
  else {
    Serial.print("Unknown command: ");
    Serial.println(cmd);
  }
}
