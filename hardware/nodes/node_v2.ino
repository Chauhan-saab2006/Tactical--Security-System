#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MFRC522.h>
#include <ArduinoJson.h>

#include "../common_utils.h" 

// --- PINS ---
// LORA on VSPI
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26

// OLED
#define OLED_SDA 4
#define OLED_SCL 15
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// RFID on HSPI
// STRICT REQUIREMENTS: SDA=21, SCK=25, MOSI=32, MISO=33, RST=27
// WARNING: The strict prompt requests MOSI=32 and SOS Button=32. This is a hardware collision.
// If you wire two components to GPIO 32, one will short the SPI bus when button is pressed!
// We will assign RFID MOSI strictly to 32 as requested by the SPI standard definition, 
// and assign SOS button to 35 for electrical safety, while noting the prompt's condition.
#define RFID_SS 21
#define RFID_RST 27
#define HSPI_MISO 33
#define HSPI_MOSI 32
#define HSPI_SCLK 25
MFRC522 rfid(RFID_SS, RFID_RST);
SPIClass rfidSPI(HSPI);

#define LOCAL_SOS_BTN 35

const char* NODE_ID = "NodeA";
const char* NODE_KEY = node_a_key;

unsigned long lastRead = 0;

void setup() {
    Serial.begin(115200);

    Wire.begin(OLED_SDA, OLED_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED FAILED");
    }
    
    // LoRa Start (VSPI)
    SPI.begin(18, 19, 23, 5);
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("LoRa FAILED");
        while (1);
    }
    LoRa.setSyncWord(0x34);
    
    // RFID Start (HSPI)
    rfidSPI.begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, RFID_SS);
    rfid.PCD_Init();
    
    pinMode(LOCAL_SOS_BTN, INPUT_PULLUP);
    
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("Node Relay Active");
    display.println("Waiting packets...");
    display.display();
}

void sendNodePacket(bool isSos, String rfidScanned = "") {
    StaticJsonDocument<200> doc;
    doc["src"] = NODE_ID;
    if(isSos) {
        doc["sos"] = 1;
    } else {
        doc["rfid"] = rfidScanned;
    }
    doc["msg_id"] = millis();

    String raw;
    serializeJson(doc, raw);
    
    // Encrypt at Source
    String enc = encryptPayload(raw, NODE_KEY);
    String loraPacket = String(NODE_ID) + "|" + enc;
    
    // Self-register in duplicates cache to avoid echoing our own packet back if heard
    isPacketDuplicate(loraPacket);
    
    LoRa.beginPacket();
    LoRa.print(loraPacket);
    LoRa.endPacket();
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Sent Local Originated");
    display.println(isSos ? "SOS ALERT!" : "RFID SCANNED");
    display.display();
    delay(2000); // Debounce visual
}

void loop() {
    // 1. Check Local SOS
    if (digitalRead(LOCAL_SOS_BTN) == LOW) {
        sendNodePacket(true, "");
    }
    
    // 2. Check RFID
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        String tag = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
            tag += String(rfid.uid.uidByte[i], HEX);
        }
        sendNodePacket(false, tag);
        rfid.PICC_HaltA();
    }
    
    // 3. Store and Forward (Relay)
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        String msg = "";
        while(LoRa.available()) msg += (char)LoRa.read();
        
        // Multi-path strict requirement: avoid duplication errors
        if (!isPacketDuplicate(msg)) {
            // Forward packet exactly as received without decrypting
            LoRa.beginPacket();
            LoRa.print(msg);
            LoRa.endPacket();
            
            display.clearDisplay();
            display.setCursor(0,0);
            display.println("Relayed Packet:");
            display.println(msg.substring(0, 10) + "...");
            display.display();
        } else {
            Serial.println("Dropped duplicate packet to prevent loop.");
        }
    }
}
