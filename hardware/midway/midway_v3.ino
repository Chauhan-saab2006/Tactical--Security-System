#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "../common_utils.h" 

// --- PINS ---
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26

#define OLED_SDA 4
#define OLED_SCL 15
Adafruit_SSD1306 display(128, 64, &Wire, -1);

#define RED_LED 27
#define YELLOW_LED 25
#define GREEN_LED 33

const char* ssid = "Veer";
const char* password = "veer1708";
const char* backendURL = "https://lifeline-production-1041.up.railway.app/api/v1/telemetry/secure"; // Railway Production

void setup() {
    Serial.begin(115200);

    pinMode(RED_LED, OUTPUT);
    pinMode(YELLOW_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    
    // Normal operation begins (Yellow)
    digitalWrite(YELLOW_LED, HIGH);

    Wire.begin(OLED_SDA, OLED_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED FAILED");
    }

    // Connect WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    // WiFi Connected -> GREEN LED
    digitalWrite(GREEN_LED, HIGH);

    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("Midway Master Active");
    display.println("Waiting for mesh...");
    display.display();

    SPI.begin(18, 19, 23, 5);
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("LoRa FAILED");
        while (1);
    }
    LoRa.setSyncWord(0x34);
}

void triggerSOSBlink() {
    // SOS Blink Red for 2 seconds
    for (int i=0; i<10; i++) {
        digitalWrite(RED_LED, HIGH);
        delay(100);
        digitalWrite(RED_LED, LOW);
        delay(100);
    }
}

void loop() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        String msg = "";
        while(LoRa.available()) msg += (char)LoRa.read();

        // Prevent echo loop bouncing back from nodes
        if (isPacketDuplicate(msg)) return;

        // Message Format Expected: "SRC|Base64Payload"
        int sepIdx = msg.indexOf('|');
        if (sepIdx > 0) {
            String srcId = msg.substring(0, sepIdx);
            String encPayload = msg.substring(sepIdx + 1);

            const char* decKey = NULL;
            if (srcId == "WB1") decKey = wristband_key;
            else if (srcId == "NodeA") decKey = node_a_key;
            else if (srcId == "NodeB") decKey = node_b_key;
            else if (srcId == "NodeC") decKey = node_c_key;

            if (decKey != NULL) {
                // Decrypt source packet (Zero Trust Bridge)
                String decRaw = decryptPayload(encPayload, decKey);
                
                // Re-encrypt exclusively with Midway Master Key for Backend
                String midEnc = encryptPayload(decRaw, master_midway_key);

                // Build Request Payload
                String jsonPost = "{\"src\":\"" + srcId + "\",\"midway_token\":\"" + midEnc + "\"}";

                if (WiFi.status() == WL_CONNECTED) {
                    WiFiClientSecure client;
                    client.setInsecure();
                    HTTPClient http;
                    http.begin(client, backendURL);
                    http.addHeader("Content-Type", "application/json");

                    int code = http.POST(jsonPost);
                    
                    if (code > 0) {
                        String response = http.getString();
                        // If backend responded that it was an SOS
                        if (response.indexOf("\"sos_triggered\":true") > 0) {
                            triggerSOSBlink();
                        }
                    }
                    http.end();
                }
            } else {
                Serial.println("Unknown Source ID from LoRa Payload.");
            }
        }
    }
}
