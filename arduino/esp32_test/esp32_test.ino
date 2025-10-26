#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#define LED_BUILTIN 2

const char* ssid = "engsci2t8pey";
const char* password = "gooncave123";

WebServer server(80);
bool blinking = false;       // state variable
unsigned long lastBlink = 0; // for timing
const int blinkInterval = 100; // ms

void handleCommand() {
  if (server.method() != HTTP_POST) { server.send(405); return; }
  String body = server.arg("plain");
  StaticJsonDocument<200> doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) { server.send(400, "application/json", "{\"error\":\"bad json\"}"); return; }
  const char* cmd = doc["cmd"];
  Serial.println(String("Received cmd: ") + cmd);

  if (String(cmd) == "cmd_locked") {
    Serial.println("CLICKED");
    blinking = !blinking;  // toggle on/off
    Serial.println(blinking ? "Blinking started" : "Blinking stopped");
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void setup() {
  delay(2000);
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

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

  if (blinking) {
    unsigned long now = millis();
    if (now - lastBlink >= blinkInterval) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      lastBlink = now;
    }
  } else {
    digitalWrite(LED_BUILTIN, LOW); // LED off when not blinking
  }
}
