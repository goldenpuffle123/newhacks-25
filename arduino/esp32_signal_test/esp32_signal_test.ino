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

// Trigger settings
const int TRIGGER_DISTANCE = 10;  // cm
const int CLOSE_TIME = 1000;       // 5 seconds

Servo servo;

const char* ssid = "engsci2t8pey";
const char* password = "gooncave123";

WebServer server(80);

const int SERVO_SPEED = 20;  // milliseconds between each degree

void moveServoSlowly(int targetAngle) {
    int currentAngle = servo.read();
    int step = (targetAngle > currentAngle) ? 1 : -1;
    
    while (currentAngle != targetAngle) {
        currentAngle += step;
        servo.write(currentAngle);
        delay(500);
    }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  servo.attach(SERVO_PIN);
  moveServoSlowly(OPEN);  // Start open

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  server.on("/command", HTTP_POST, handleCommand);
  server.begin();
  Serial.println("HTTP server started");
}



void loop() {
  server.handleClient();
  int distance = getDistance();
  Serial.println(distance);
  
//  if (distance < TRIGGER_DISTANCE) {
//    moveServoSlowly(CLOSE);
//    delay(CLOSE_TIME);
//    servo.write(OPEN);
//  }
  
  delay(100);
}

void handleCommand() {
  if (server.method() != HTTP_POST) { server.send(405); return; }
  String body = server.arg("plain");
  StaticJsonDocument<200> doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) { server.send(400, "application/json", "{\"error\":\"bad json\"}"); return; }
  const char* cmd = doc["cmd"];
  Serial.println(String("Received cmd: ") + cmd);

  if (String(cmd) == "cmd_locked") {
    moveServoSlowly(CLOSED);
  }
  else if (String(cmd) == "cmd_unlocked") {
    moveServoSlowly(OPEN);
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}
