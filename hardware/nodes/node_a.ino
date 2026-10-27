#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRa.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>

// ================= DEVICE ID =================
const char *NODE_ID = "Node-A";

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

// ================= RFID (MFRC522) =================
#define RFID_SS 21
#define RFID_RST 22
MFRC522 rfid(RFID_SS, RFID_RST);

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

  // ===== SPI Bus Management =====
  // Set all SS pins to high before starting SPI to avoid contention
  pinMode(LORA_SS, OUTPUT);
  pinMode(RFID_SS, OUTPUT);
  digitalWrite(LORA_SS, HIGH);
  digitalWrite(RFID_SS, HIGH);

  // Initialize SPI with explicit pins matching your hardware
  // (SCK, MISO, MOSI, SS)
  SPI.begin(18, 19, 23, LORA_SS);

  // ===== LoRa Setup =====
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Init Failed! Check connections or frequency.");
    display.clearDisplay();
    display.setCursor(0, 20);
    display.println("LoRa FAILED");
    display.println("Check Pins: 5,14,26");
    display.display();
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

  // RFID Init
  rfid.PCD_Init();
  Serial.println("RFID Reader Ready");
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

  // Check for RFID Tags
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    Serial.print("Tag Detected UID: ");
    Serial.println(uid);

    // Using user_1234 as requested
    const char *userID = "user_1234";

    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("CHECKPOINT!");
    display.println("User Scanned");
    display.display();

    // Transmit Checkpoint Packet: CP:user_id:node_id
    LoRa.beginPacket();
    LoRa.print("CP:");
    LoRa.print(userID);
    LoRa.print(":");
    LoRa.print(NODE_ID);
    LoRa.endPacket();

    Serial.println("Checkpoint Transmitted");

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    delay(2000); // Prevent multi-scan

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("NODE: ");
    display.println(NODE_ID);
    display.println("LoRa READY");
    display.display();
  }
}
