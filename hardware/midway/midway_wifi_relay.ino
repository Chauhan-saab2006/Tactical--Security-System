#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h> // WebSockets by Markus Sattler
#include <WiFi.h>
#include <Wire.h>


// ================= WIFI SETTINGS =================
const char *ssid = "Veer";
const char *password = "veer1708";

// Backend WebSocket URL
const char *wsHost = "10.124.177.46";
const int wsPort = 8000;
const char *wsPath = "/ws/sos";

// ================= OLED SETTINGS =================
#define OLED_SDA 4
#define OLED_SCL 15
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= TRAFFIC LEDS =================
#define RED_LED 27
#define YELLOW_LED 25
#define GREEN_LED 33

WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
  case WStype_DISCONNECTED:
    Serial.println("[WSc] Disconnected!");
    digitalWrite(YELLOW_LED, LOW);
    break;
  case WStype_CONNECTED:
    Serial.print("[WSc] Connected to url: ");
    Serial.println((char *)payload);
    digitalWrite(YELLOW_LED, HIGH); // Yellow LED means WebSocket OK
    break;
  case WStype_TEXT:
    Serial.printf("[WSc] get text: %s\n", payload);
    handleIncomingMessage((char *)payload);
    break;
  }
}

void setup() {
  Serial.begin(115200);

  // LED Setup
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, HIGH); // Green ON = Power Wait

  // OLED Init
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED INIT FAILED");
    for (;;)
      ;
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Midway Relay v3");
  display.println("Connecting WiFi...");
  display.display();

  // WiFi Connect
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected");
  display.println(WiFi.localIP());
  display.display();

  // WebSocket Server setup
  webSocket.begin(wsHost, wsPort, wsPath);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);

  Serial.println("System Ready - Listening for SOS via WebSocket");
}

void loop() { webSocket.loop(); }

void handleIncomingMessage(char *payload) {
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char *type = doc["type"];
  bool sos = doc["sos"];
  const char *device_id = doc["device_id"];

  if (String(type) == "sos_alert" || sos == true) {
    Serial.printf("INSTANT SOS RECEIVED from %s!\n", device_id);
    triggerAlert(device_id);
  }
}

void triggerAlert(const char *deviceId) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.println("SOS ALERT!");
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("FROM: ");
  display.println(deviceId);
  display.display();

  digitalWrite(GREEN_LED, LOW);

  // Flash RED LED and alert
  for (int i = 0; i < 15; i++) {
    digitalWrite(RED_LED, HIGH);
    delay(100);
    digitalWrite(RED_LED, LOW);
    delay(100);
  }

  digitalWrite(GREEN_LED, HIGH);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("System Ready");
  display.println("Waiting for alerts...");
  display.display();
}
