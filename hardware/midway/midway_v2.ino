#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LoRa.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>

// ================= WIFI =================
const char *ssid = "Veer";
const char *password = "veer1708";

// Backend Production URL (Railway)
const char *backendURL = "https://lifeline-production-1041.up.railway.app/api/v1/telemetry";
const char *deviceApiKey = "antigravity_secret_123";


// ================= LoRa Pins =================
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26

// ================= OLED =================
#define OLED_SDA 4
#define OLED_SCL 15
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= Traffic LEDs =================
#define RED_LED 27
#define YELLOW_LED 25
#define GREEN_LED 33

// =================================================

void connectWiFi() {
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connecting WiFi");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected");
  display.println(WiFi.localIP());
  display.display();
  delay(1000);
}

// ================= Node Coordinates =================
struct NodeData {
  const char *id;
  float lat;
  float lon;
};

NodeData nodes[] = {{"Node-A", 30.268841, 77.993322},
                    {"Node-B", 30.268841, 77.993322},
                    {"Node-C", 30.270905, 77.993004}};

void sendSOSToBackend(const char *nodeID, float lat, float lon,
                      bool isSOS = true, const char *userID = "") {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, backendURL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Device-Key", deviceApiKey);

    // Dynamic JSON Payload - Simplified for robustness
    String jsonPayload = "{";
    jsonPayload += "\"device_id\":\"" + String(nodeID) + "\",";
    jsonPayload += "\"latitude\":" + String(lat, 6) + ",";
    jsonPayload += "\"longitude\":" + String(lon, 6) + ",";
    jsonPayload += "\"pulse\":72,";
    jsonPayload += "\"sos\":" + String(isSOS ? "true" : "false") + ",";
    jsonPayload +=
        "\"is_checkpoint\":" + String(!isSOS ? "true" : "false") + ",";
    jsonPayload += "\"user_id\":\"" + String(userID) + "\"";
    jsonPayload += "}";

    Serial.print("Debug Payload: ");
    Serial.println(jsonPayload);

    int httpResponseCode = http.POST(jsonPayload);

    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    Serial.println("ERROR: WiFi Disconnected. Cannot send to backend!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WIFI LOST!");
    display.display();
  }
}

// =================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  // ===== OLED INIT FIRST =====
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED INIT FAILED");
    while (1)
      ;
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Midway Panel v2");
  display.println("Backend Ready");
  display.display();
  delay(1000);

  // ===== LED Setup =====
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  // ===== Connect WiFi =====
  connectWiFi();

  // ===== LoRa Setup =====
  SPI.begin(18, 19, 23);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa INIT FAILED");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("LoRa FAILED");
    display.display();
    while (1)
      ;
  }

  LoRa.setSyncWord(0x34);
  LoRa.setSpreadingFactor(7);
  LoRa.enableCrc();

  digitalWrite(YELLOW_LED, HIGH); // LoRa OK
  digitalWrite(GREEN_LED, HIGH);  // Normal

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("LoRa READY");
  display.println("Waiting SOS...");
  display.display();

  Serial.println("System Ready - v2 Backend Connected");
}

// =================================================

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }
    incoming.trim(); // Remove any whitespace/newlines

    Serial.print("Received (Trimmed): ");
    Serial.println(incoming);

    // 1. Handle Checkpoint Packets (Format: CP:user_id:node_id)
    if (incoming.startsWith("CP:")) {
      int firstColon = incoming.indexOf(':');
      int secondColon = incoming.indexOf(':', firstColon + 1);

      String userID = incoming.substring(firstColon + 1, secondColon);
      String nodeID = incoming.substring(secondColon + 1);

      Serial.print("CHECKPOINT DETECTED - User: ");
      Serial.print(userID);
      Serial.print(" at ");
      Serial.println(nodeID);

      // Find node coordinates
      float lat = 30.8000;
      float lon = 76.8500;
      for (int i = 0; i < 3; i++) {
        if (nodeID == nodes[i].id) {
          lat = nodes[i].lat;
          lon = nodes[i].lon;
          break;
        }
      }

      display.clearDisplay();
      display.setCursor(0, 10);
      display.println("CHECKPOINT");
      display.println(userID);
      display.println(nodeID);
      display.display();

      // Forward to backend as non-SOS checkpoint
      sendSOSToBackend(nodeID.c_str(), lat, lon, false, userID.c_str());

      delay(2000);
      display.clearDisplay();
      display.println("Waiting SOS...");
      display.display();
      return; // Exit loop for this packet
    }

    // 2. Handle SOS Packets
    // Default to midway panel if generic SOS or unknown ID
    const char *targetID = "midway_panel";
    float targetLat = 30.8000;
    float targetLon = 76.8500;
    bool foundNode = false;

    // Check if incoming matches any Node ID (e.g., "Node-B")
    for (int i = 0; i < 3; i++) {
      if (incoming == nodes[i].id) {
        targetID = nodes[i].id;
        targetLat = nodes[i].lat;
        targetLon = nodes[i].lon;
        foundNode = true;
        break;
      }
    }

    if (incoming == "SOS" || foundNode) {
      Serial.print("ALERT FROM: ");
      Serial.println(targetID);

      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 10);
      display.println("SOS!");
      display.setTextSize(1);
      display.setCursor(0, 40);
      display.println(targetID);
      display.display();

      digitalWrite(GREEN_LED, LOW);

      // 1. Trigger Local Webpage Backend with Dynamic Coords
      sendSOSToBackend(targetID, targetLat, targetLon, true);



      // Mechanical Alarm Feedback
      digitalWrite(RED_LED, HIGH);
      for (int i = 0; i < 10; i++) {
        digitalWrite(RED_LED, LOW);
        delay(120);
        digitalWrite(RED_LED, HIGH);
        delay(120);
      }
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, HIGH);

      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Waiting SOS...");
      display.display();
    }
  }
}
