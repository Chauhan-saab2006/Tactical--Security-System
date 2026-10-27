import os

wristband_template = """#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRa.h>
#include <SPI.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include "mbedtls/aes.h"

// ================= DEVICE CONFIG =================
const char *NODE_ID = "Wristband";
unsigned char aes_key[32] = {'W','R','I','S','T','B','A','N','D','K','E','Y','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0'};
unsigned char aes_iv[16]  = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

// ================= PINS =================
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26
#define OLED_SDA 4
#define OLED_SCL 15
Adafruit_SSD1306 display(128, 64, &Wire, -1);
#define SOS_BUTTON 32

#define GPS_RX 16
#define GPS_TX 17
TinyGPSPlus gps;

#define PULSE_PIN 34

unsigned long lastSend = 0;
unsigned long lastPulseRead = 0;
bool sosActive = false;
int bpm = 75;

// Mesh loop prevention
String recentMsgIDs[15];
int msgIdx = 0;

void addMsgSeen(String mid) {
  recentMsgIDs[msgIdx] = mid;
  msgIdx = (msgIdx + 1) % 15;
}

void forwardMeshPacket(String packet) {
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();
}

String encryptToHex(String message) {
  int input_len = message.length();
  int padded_len = ((input_len / 16) + 1) * 16;
  unsigned char input[padded_len];
  uint8_t output[padded_len];
  memset(input, 0, padded_len);
  memcpy(input, message.c_str(), input_len);
  uint8_t pad_val = padded_len - input_len;
  for (int i = input_len; i < padded_len; i++) input[i] = pad_val;

  unsigned char iv_copy[16];
  memcpy(iv_copy, aes_iv, 16);

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, aes_key, 256);
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded_len, iv_copy, input, output);
  mbedtls_aes_free(&aes);

  String hexStr = "";
  for(int i=0; i<padded_len; i++) {
    if(output[i] < 0x10) hexStr += "0";
    hexStr += String(output[i], HEX);
  }
  return hexStr;
}

void sendWristbandData() {
  String latStr = "29.027916";
  String lonStr = "77.059315";
  if (gps.location.isValid() && gps.location.age() < 5000) {
    latStr = String(gps.location.lat(), 6);
    lonStr = String(gps.location.lng(), 6);
  }

  String payload = "BPM:" + String(bpm) + "|LAT:" + latStr + "|LON:" + lonStr + "|SOS:" + (sosActive ? "1" : "0");
  String mid = "M" + String(random(10000, 99999));
  addMsgSeen(mid);
  String hexStr = encryptToHex(payload);
  String packet = mid + ":" + String(NODE_ID) + ":" + hexStr;
  
  display.clearDisplay(); display.setCursor(0,0);
  display.println(sosActive ? "EMERGENCY!" : "Sending Data...");
  display.println("BPM: " + String(bpm));
  display.display();

  forwardMeshPacket(packet);
  Serial.println("Sent: " + packet);
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX); // GPS

  delay(1000);
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) while(1);
  display.clearDisplay(); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("WRISTBAND INIT"); display.display();

  pinMode(LORA_SS, OUTPUT);
  digitalWrite(LORA_SS, HIGH);
  SPI.begin(18, 19, 23, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) while(1);
  LoRa.setSyncWord(0x34);

  pinMode(SOS_BUTTON, INPUT_PULLUP);
  randomSeed(analogRead(0));
}

void readPulse() {
    int sensorValue = analogRead(PULSE_PIN);
    // Simple mock logic for pulse
    if (sensorValue < 100) bpm = 0; // Sensor failed/disconnected
    else {
      // simulate realistic varying BPM or parse sensor if we had lib
      bpm = map(sensorValue, 0, 4095, 50, 150); 
    }
}

void loop() {
  while (Serial1.available() > 0) gps.encode(Serial1.read());

  unsigned long currentMillis = millis();

  if (currentMillis - lastPulseRead >= 1000) {
    lastPulseRead = currentMillis;
    readPulse();

    bool physicalSos = (digitalRead(SOS_BUTTON) == LOW);
    bool autoSos = (bpm > 0 && (bpm < 40 || bpm > 120)); // AUTO SOS

    if (physicalSos || autoSos) {
      if (!sosActive) {
          sosActive = true;
          sendWristbandData(); // send immediately on trigger
      }
    } else {
      sosActive = false;
    }
  }

  // Send routine telemetry every 10 seconds
  if (currentMillis - lastSend >= 10000) {
    lastSend = currentMillis;
    sendWristbandData();
  }
}
"""

midway_template = """#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRa.h>
#include <SPI.h>
#include <Wire.h>
#include "mbedtls/aes.h"
#include <ArduinoJson.h>

// ================= CONFIG =================
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";
const char* backend_url = "http://192.168.1.100:8001/decoder-update";

unsigned char midway_key[32] = {'M','I','D','W','A','Y','K','E','Y','A','E','S','1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F','1','2','3','4'};
unsigned char aes_iv[16]     = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

// Node Keys database mapped by OrigID
unsigned char key_wrist[32] = {'W','R','I','S','T','B','A','N','D','K','E','Y','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0'};
unsigned char key_nodeA[32] = {'N','O','D','E','A','K','E','Y','A','E','S','1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F','1','2','3','4','5'};
unsigned char key_nodeB[32] = {'N','O','D','E','B','K','E','Y','A','E','S','1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F','1','2','3','4','5'};
unsigned char key_nodeC[32] = {'N','O','D','E','C','K','E','Y','A','E','S','1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F','1','2','3','4','5'};

// Status LEDs
#define LED_RED 27
#define LED_YELLOW 25
#define LED_GREEN 33
unsigned long redLedOffTime = 0;

// LoRa & OLED
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26
#define OLED_SDA 4
#define OLED_SCL 15
Adafruit_SSD1306 display(128, 64, &Wire, -1);

String recentMsgIDs[30];
int msgIdx = 0;

bool isMsgSeen(String mid) {
  for(int i=0; i<30; i++) {
    if(recentMsgIDs[i] == mid) return true;
  }
  return false;
}
void addMsgSeen(String mid) {
  recentMsgIDs[msgIdx] = mid;
  msgIdx = (msgIdx + 1) % 30;
}

byte hexCharToByte(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}

void hexStringToBytes(String hex, unsigned char* outBytes, int outLen) {
  for(int i=0; i<outLen; i++) {
    outBytes[i] = (hexCharToByte(hex[i*2]) << 4) | hexCharToByte(hex[i*2 + 1]);
  }
}

String decryptHex(String hexMsg, unsigned char* key) {
  int len = hexMsg.length() / 2;
  unsigned char encrypted[len];
  hexStringToBytes(hexMsg, encrypted, len);

  unsigned char decrypted[len];
  unsigned char iv_copy[16];
  memcpy(iv_copy, aes_iv, 16);

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_dec(&aes, key, 256);
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, len, iv_copy, encrypted, decrypted);
  mbedtls_aes_free(&aes);

  // Strip PKCS7 padding
  uint8_t pad = decrypted[len - 1];
  int clearLen = len - pad;
  if (clearLen < 0 || clearLen > len) clearLen = len;

  String clearText = "";
  for(int i=0; i<clearLen; i++) clearText += (char)decrypted[i];
  return clearText;
}

String encryptToHex(String message, unsigned char* key) {
  int input_len = message.length();
  int padded_len = ((input_len / 16) + 1) * 16;
  unsigned char input[padded_len];
  uint8_t output[padded_len];
  memset(input, 0, padded_len);
  memcpy(input, message.c_str(), input_len);
  uint8_t pad_val = padded_len - input_len;
  for (int i = input_len; i < padded_len; i++) input[i] = pad_val;

  unsigned char iv_copy[16];
  memcpy(iv_copy, aes_iv, 16);

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, key, 256);
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded_len, iv_copy, input, output);
  mbedtls_aes_free(&aes);

  String hexStr = "";
  for(int i=0; i<padded_len; i++) {
    if(output[i] < 0x10) hexStr += "0";
    hexStr += String(output[i], HEX);
  }
  return hexStr;
}

void forwardToBackend(String origId, String decryptedPayload) {
  String reEncrypted = encryptToHex(decryptedPayload, midway_key);
  
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(backend_url);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<512> doc;
    doc["encrypted_data"] = reEncrypted;
    doc["decrypted_data"] = decryptedPayload; // for the UI to show immediately
    doc["device_id"] = origId;
    doc["aes_key"] = "MIDWAYKEYAES1234567890ABCDEF1234";

    String requestBody;
    serializeJson(doc, requestBody);
    int httpResponseCode = http.POST(requestBody);
    Serial.println("POST code: " + String(httpResponseCode));
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(LED_YELLOW, HIGH); 

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) while(1);
  display.clearDisplay(); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("MIDWAY INIT"); display.display();

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  digitalWrite(LED_GREEN, HIGH); 
  display.println("WIFI CONNECTED"); display.display();

  SPI.begin(18, 19, 23, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) while(1);
  LoRa.setSyncWord(0x34);
}

void loop() {
  if (millis() > redLedOffTime) {
    digitalWrite(LED_RED, LOW);
  } else {
    // blink it
    digitalWrite(LED_RED, (millis() / 200) % 2);
  }

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = "";
    while(LoRa.available()) incoming += (char)LoRa.read();
    
    int firstColon = incoming.indexOf(':');
    int secondColon = incoming.indexOf(':', firstColon + 1);

    if(firstColon > 0 && secondColon > firstColon) {
      String msgId = incoming.substring(0, firstColon);
      String origId = incoming.substring(firstColon + 1, secondColon);
      String hexData = incoming.substring(secondColon + 1);

      if(!isMsgSeen(msgId)) {
        addMsgSeen(msgId);
        Serial.println("Midway Rcvd: " + origId);

        unsigned char* targetKey = NULL;
        if(origId == "Wristband") targetKey = key_wrist;
        else if(origId == "Node-A") targetKey = key_nodeA;
        else if(origId == "Node-B") targetKey = key_nodeB;
        else if(origId == "Node-C") targetKey = key_nodeC;

        if (targetKey != NULL) {
          String decrypted = decryptHex(hexData, targetKey);
          Serial.println("Decrypted: " + decrypted);

          if(decrypted.indexOf("SOS") >= 0 || decrypted.indexOf("BPM:") >= 0) {
              if (decrypted.indexOf("SOS:1") >= 0 || decrypted == "SOS") {
                 redLedOffTime = millis() + 2000;
              }
          }

          display.clearDisplay(); display.setCursor(0,0);
          display.println("PAYLOAD: " + origId);
          display.display();

          forwardToBackend(origId, decrypted);
        }
      }
    }
  }
}
"""

with open(r"d:\mess_network\LIFE-LINK-NETWORK\hardware\wrist_band\wristband_mesh.ino", "w") as f:
    f.write(wristband_template)
print("Wrote wristband")

with open(r"d:\mess_network\LIFE-LINK-NETWORK\hardware\midway\midway_mesh.ino", "w") as f:
    f.write(midway_template)
print("Wrote midway")
