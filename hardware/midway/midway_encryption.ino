#include "mbedtls/base64.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>
#include <LoRa.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

// ================= WIFI =================
const char *ssid = "Veer";
const char *password = "veer1708";

// Backend SECURE endpoint (Railway Production)
const char *backendURL = "https://lifeline-production-1041.up.railway.app/api/v1/telemetry/secure";
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
  display.println("Midway SECURE v3");
  display.println("Zero-Knowledge Relay");
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
  display.println("LoRa READY (E2EE)");
  display.println("Relaying Packets...");
  display.display();

  Serial.println("System Ready - Forwarding Data using E2EE");
}

// =================================================

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    uint8_t buffer[256];
    int len = 0;
    while (LoRa.available() && len < 256) {
      buffer[len++] = (uint8_t)LoRa.read();
    }

    // Base64 Encode the RAW ENCRYPTED LoRa buffer
    size_t out_len;
    unsigned char b64_output[512];
    memset(b64_output, 0, sizeof(b64_output));

    mbedtls_base64_encode(b64_output, sizeof(b64_output), &out_len, buffer,
                          len);

    String securePayload =
        "{\"encrypted_data\":\"" + String((char *)b64_output) + "\"}";
    Serial.print("Forwarding payload to backend: ");
    Serial.println(securePayload);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("sos recived");
    display.println("forwarding to ...");
    display.display();

    if (WiFi.status() == WL_CONNECTED) {
      WiFiClientSecure client;
      client.setInsecure(); // Skip certificate verification
      HTTPClient http;
      http.begin(client, backendURL);
      http.addHeader("Content-Type", "application/json");
      http.addHeader("X-Device-Key", deviceApiKey);

      int httpResponseCode = http.POST(securePayload);
      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Backend Response: " + response);

        // Check if the backend detected an SOS
        if (response.indexOf("\"is_sos\": true") > 0 ||
            response.indexOf("\"is_sos\":true") > 0 ||
            response.indexOf("\"is_sos\": True") > 0) {
          display.clearDisplay();
          display.setTextSize(2);
          display.setCursor(0, 10);
          display.println("SOS ALERT!");
          display.setTextSize(1);
          display.display();

          digitalWrite(GREEN_LED, LOW);

          // Fast blinking for 3 seconds (30 loops of 100ms)
          for (int i = 0; i < 30; i++) {
            digitalWrite(RED_LED, HIGH);
            delay(50);
            digitalWrite(RED_LED, LOW);
            delay(50);
          }
          digitalWrite(GREEN_LED, HIGH);

        } else if (response.indexOf("\"is_checkpoint\": true") > 0 ||
                   response.indexOf("\"is_checkpoint\":true") > 0 ||
                   response.indexOf("\"is_checkpoint\": True") > 0) {
          display.clearDisplay();
          display.setCursor(0, 10);
          display.println("CHECKPOINT OK");
          display.display();
          delay(2000);
        }
      } else {
        Serial.print("HTTP Error: ");
        Serial.println(httpResponseCode);
      }
      http.end();
    } else {
      Serial.println("WiFi Disconnected!");
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("LoRa E2EE READY");
    display.println("Waiting Packets...");
    display.display();
  }
}
