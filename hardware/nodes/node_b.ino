#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRa.h>
#include <SPI.h>
#include <Wire.h>


// ================= DEVICE ID =================
const char *NODE_ID = "Node-B";

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

// ================= SOS Button =================
#define SOS_BUTTON 32

void setup() {
  Serial.begin(115200);
  delay(1000);

  // OLED
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
  display.print("NODE: ");
  display.println(NODE_ID);
  display.println("Initializing LoRa...");
  display.display();

  // LoRa Init
  SPI.begin(18, 19, 23, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Init Failed");
    while (1)
      ;
  }

  LoRa.setSyncWord(0x34);
  LoRa.setSpreadingFactor(7);
  LoRa.enableCrc();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("NODE: ");
  display.println(NODE_ID);
  display.println("LoRa READY");
  display.display();

  pinMode(SOS_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(SOS_BUTTON) == LOW) {
    Serial.print("Sending SOS from ");
    Serial.println(NODE_ID);

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.println("SOS!");
    display.setTextSize(1);
    display.println("Transmitting id...");
    display.display();

    // Transmit NODE ID
    LoRa.beginPacket();
    LoRa.print(NODE_ID);
    LoRa.endPacket();

    delay(2000); // Debounce and show status

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("NODE: ");
    display.println(NODE_ID);
    display.println("LoRa READY");
    display.display();
  }
}
