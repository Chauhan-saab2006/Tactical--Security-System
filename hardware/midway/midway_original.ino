#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================= WIFI =================
const char* ssid = "Veer";
const char* password = "veer1708";

// ================= LoRa Pins =================
#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 26

// ================= OLED =================
#define OLED_SDA 4
#define OLED_SCL 15
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= Traffic LEDs =================
#define RED_LED     27
#define YELLOW_LED  25
#define GREEN_LED   33

// =================================================

void connectWiFi() {
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Connecting WiFi");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("WiFi Connected");
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
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Control Panel");
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
    display.setCursor(0,0);
    display.println("LoRa FAILED");
    display.display();

    while (1);
  }

  LoRa.setSyncWord(0x34);
  LoRa.setSpreadingFactor(7);
  LoRa.enableCrc();

  digitalWrite(YELLOW_LED, HIGH);   // LoRa OK
  digitalWrite(GREEN_LED, HIGH);    // Normal

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("LoRa READY");
  display.println("Waiting SOS...");
  display.display();

  Serial.println("System Ready");
}

// =================================================

void loop() {

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    String incoming = "";

    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }

    Serial.print("Received: ");
    Serial.println(incoming);

    if (incoming == "SOS") {

      Serial.println("SOS RECEIVED");

      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0,20);
      display.println("SOS!");
      display.display();

      digitalWrite(GREEN_LED, LOW);



        // RED turns ON instantly
        digitalWrite(RED_LED, HIGH);

        // Fast blink 10 times
        for(int i = 0; i < 10; i++) {
          digitalWrite(RED_LED, LOW);
          delay(120);
          digitalWrite(RED_LED, HIGH);
          delay(120);
}

// Final OFF
digitalWrite(RED_LED, LOW);
      }

      digitalWrite(GREEN_LED, HIGH);

      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0,0);
      display.println("Waiting SOS...");
      display.display();
    }
}
