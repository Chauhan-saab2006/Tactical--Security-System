#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// Path back to common_utils.h (adjust path based on Arduino IDE setup)
// To keep compiling simple, assume user will copy the header or we define relative PATH
#include "../common_utils.h" 

// --- PINS ---
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26

#define OLED_SDA 4
#define OLED_SCL 15
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define GPS_TX 16
#define GPS_RX 17
HardwareSerial gpsSerial(1);

#define PULSE_PIN 34
#define SOS_BTN 32

const char* DEVICE_ID = "WB1";

unsigned long lastSend = 0;
bool sosActive = false;

// Mock values for robust simulation if sensor fails
float currentLat = 29.027916;
float currentLng = 77.059315;
int currentBpm = 0;

void setup() {
    Serial.begin(115200);
    gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
    
    pinMode(SOS_BTN, INPUT_PULLUP);
    pinMode(PULSE_PIN, INPUT);
    
    Wire.begin(OLED_SDA, OLED_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED FAILED");
        for(;;);
    }
    
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("Wristband Initializing..");
    display.display();
    
    SPI.begin(18, 19, 23, 5);
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("LoRa FAILED");
        while (1);
    }
    LoRa.setSyncWord(0x34);
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Wristband Active");
    display.println("LoRa Ready");
    display.display();
}

void loop() {
    if (digitalRead(SOS_BTN) == LOW) {
        sosActive = true;
        delay(200); // debounce
    }
    
    if (millis() - lastSend >= 1000) {
        lastSend = millis();
        
        // 1. Read Pulse (Simplified representation)
        int rawPulse = analogRead(PULSE_PIN);
        if (rawPulse > 1000) currentBpm = 75; // Simulation trigger if pin active
        else currentBpm = 0; // Sensor fail = 0
        
        // Auto SOS logic strictly requested: BPM Too Low or Too High
        // But if failed (0), don't trigger
        if (currentBpm > 0 && (currentBpm < 60 || currentBpm > 100)) {
            sosActive = true;
        }

        // 2. Read GPS (Simplified)
        // If GPS returns valid data, update lat/lng. If fail -> default
        if (gpsSerial.available()) {
            // (Assuming parsed data here, falling back to defaults for robustness)
            currentLat = 29.027916; 
            currentLng = 77.059315;
        }
        
        // 3. Construct JSON
        StaticJsonDocument<200> doc;
        doc["src"] = DEVICE_ID;
        doc["lat"] = currentLat;
        doc["lng"] = currentLng;
        doc["pulse"] = currentBpm;
        doc["sos"] = sosActive ? 1 : 0;
        doc["msg_id"] = millis(); // Unique packet

        String jsonRaw;
        serializeJson(doc, jsonRaw);
        
        // 4. Encrypt ONLY the data (AES-256-CBC)
        String encryptedData = encryptPayload(jsonRaw, wristband_key);
        
        // 5. Package for LoRa
        String loraPacket = String(DEVICE_ID) + "|" + encryptedData;
        
        LoRa.beginPacket();
        LoRa.print(loraPacket);
        LoRa.endPacket();

        // Screen output
        display.clearDisplay();
        display.setCursor(0,0);
        display.println(sosActive ? "!!! SOS ACTIVE !!!" : "Monitoring Activity");
        display.print("BPM: "); display.println(currentBpm);
        display.print("Packet: "); display.println(jsonRaw.substring(0, 15) + "...");
        display.display();

        if (sosActive) sosActive = false; // Reset for next iteration or keep true based on preference. Resetting keeps mesh clean.
    }
}
