#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRa.h>
#include <SPI.h>
#include <Wire.h>
#include "mbedtls/gcm.h"
#include <ArduinoJson.h>

// ================= CONFIG =================
const char* ssid = "veer";
const char* password = "veer1708";

// Backend Production URL (Railway)
const char* backendURL = "https://lifeline-production-1041.up.railway.app/api/v1/telemetry";

unsigned char midway_key[32] = {'M','I','D','W','A','Y','K','E','Y','A','E','S','1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F','1','2','3','4'};
// Binary: 01001101 01001001 01000100 01010111 01000001 01011001 01001011 01000101 01011001 01000001 01000101 01010011 00110001 00110010 00110011 00110100 00110101 00110110 00110111 00111000 00111001 00110000 01000001 01000010 01000011 01000100 01000101 01000110 00110001 00110010 00110011 00110100

// Node Keys database mapped by OrigID
unsigned char key_wrist[32] = {'W','R','I','S','T','B','A','N','D','K','E','Y','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0'};
// Binary: 01010111 01010010 01001001 01010011 01010100 01000010 01000001 01001110 01000100 01001011 01000101 01011001 00110001 00110010 00110011 00110100 00110101 00110110 00110111 00111000 00111001 00110000 00110001 00110010 00110011 00110100 00110101 00110110 00110111 00111000 00111001 00110000
unsigned char key_nodeA[32] = {'N','O','D','E','A','K','E','Y','A','E','S','1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F','1','2','3','4','5'};
// Binary: 01001110 01001111 01000100 01000101 01000001 01001011 01000101 01011001 01000001 01000101 01010011 00110001 00110010 00110011 00110100 00110101 00110110 00110111 00111000 00111001 00110000 01000001 01000010 01000011 01000100 01000101 01000110 00110001 00110010 00110011 00110100 00110101
unsigned char key_nodeB[32] = {'N','O','D','E','B','K','E','Y','A','E','S','1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F','1','2','3','4','5'};
// Binary: 01001110 01001111 01000100 01000101 01000010 01001011 01000101 01011001 01000001 01000101 01010011 00110001 00110010 00110011 00110100 00110101 00110110 00110111 00111000 00111001 00110000 01000001 01000010 01000011 01000100 01000101 01000110 00110001 00110010 00110011 00110100 00110101
unsigned char key_nodeC[32] = {'N','O','D','E','C','K','E','Y','A','E','S','1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F','1','2','3','4','5'};
// Binary: 01001110 01001111 01000100 01000101 01000011 01001011 01000101 01011001 01000001 01000101 01010011 00110001 00110010 00110011 00110100 00110101 00110110 00110111 00111000 00111001 00110000 01000001 01000010 01000011 01000100 01000101 01000110 00110001 00110010 00110011 00110100 00110101

// Status LEDs
#define LED_RED 27
#define LED_YELLOW 25
#define LED_GREEN 33
unsigned long sosBlinkUntil = 0;
bool isRedLedOn = false;

// LoRa & OLED
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26
#define OLED_SDA 4
#define OLED_SCL 15
Adafruit_SSD1306 display(128, 64, &Wire, -1);

struct NonceCache {
  String nonce;
  unsigned long timestamp;
};
NonceCache recentNonces[30];
int nonceIdx = 0;

unsigned long lastHeartbeatTime = 0;

const char* DEVICE_API_KEY = "antigravity_secret_123";

bool isNonceSeen(String nonce) {
  unsigned long currentMillis = millis();
  for(int i=0; i<30; i++) {
    if(recentNonces[i].nonce == nonce) {
      if (currentMillis - recentNonces[i].timestamp < 60000) {
        return true; 
      }
    }
  }
  return false;
}

void addNonceSeen(String nonce) {
  recentNonces[nonceIdx].nonce = nonce;
  recentNonces[nonceIdx].timestamp = millis();
  nonceIdx = (nonceIdx + 1) % 30;
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

String decryptGCM(String hexCipher, String hexIV, String hexTag, unsigned char* key) {
  int cipherLen = hexCipher.length() / 2;
  int ivLen = hexIV.length() / 2;
  int tagLen = hexTag.length() / 2;

  if (cipherLen == 0 || ivLen != 12 || tagLen != 16) {
    Serial.println("Invalid GCM lengths");
    return "";
  }

  unsigned char* cipherText = (unsigned char*)malloc(cipherLen);
  unsigned char iv[12];
  unsigned char tag[16];
  unsigned char* plainText = (unsigned char*)malloc(cipherLen);

  if (!cipherText || !plainText) {
    if (cipherText) free(cipherText);
    if (plainText) free(plainText);
    Serial.println("Memory allocation failed for GCM");
    return "";
  }

  hexStringToBytes(hexCipher, cipherText, cipherLen);
  hexStringToBytes(hexIV, iv, 12);
  hexStringToBytes(hexTag, tag, 16);

  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);
  mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256);

  int ret = mbedtls_gcm_auth_decrypt(&gcm, cipherLen,
                                     iv, 12,
                                     NULL, 0,
                                     tag, 16,
                                     cipherText, plainText);
  mbedtls_gcm_free(&gcm);

  free(cipherText);

  if (ret != 0) {
    Serial.println("GCM Authentication Failed!");
    free(plainText);
    return "";
  }

  String clearText = "";
  for(int i=0; i<cipherLen; i++) {
    clearText += (char)plainText[i];
  }

  free(plainText);
  return clearText;
}

bool parseDecryptedPayload(String origId, String decryptedPayload, float &lat, float &lon, int &pulse, bool &sos) {
  lat = 0.0;
  lon = 0.0;
  pulse = 0;
  sos = false;

  // Use base locations based on originating node
  if (origId == "Node-A") { lat = 30.268841; lon = 77.993322; }
  else if (origId == "Node-B") { lat = 30.268841; lon = 77.993322; }
  else if (origId == "Node-C") { lat = 30.270905; lon = 77.993004; }

  if (decryptedPayload.startsWith("RNR")) {
    sos = true;
    return true; // Simple SOS payload
  }

  int bpmIdx = decryptedPayload.indexOf("BPM:");
  int latIdx = decryptedPayload.indexOf("LAT:");
  int lonIdx = decryptedPayload.indexOf("LON:");
  int rnrIdx = decryptedPayload.indexOf("RNR:");

  if (bpmIdx >= 0 && latIdx >= 0 && lonIdx >= 0) {
    int bpmEnd = decryptedPayload.indexOf('|', bpmIdx);
    int latEnd = decryptedPayload.indexOf('|', latIdx);
    int lonEnd = decryptedPayload.indexOf('|', lonIdx);
    if (bpmEnd < 0) bpmEnd = decryptedPayload.length();
    if (latEnd < 0) latEnd = decryptedPayload.length();
    if (lonEnd < 0) lonEnd = decryptedPayload.length();

    pulse = decryptedPayload.substring(bpmIdx + 4, bpmEnd).toInt();
    lat = decryptedPayload.substring(latIdx + 4, latEnd).toFloat();
    lon = decryptedPayload.substring(lonIdx + 4, lonEnd).toFloat();

    if (rnrIdx >= 0) {
      int rnrEnd = decryptedPayload.indexOf('|', rnrIdx);
      if (rnrEnd < 0) rnrEnd = decryptedPayload.length();
      String rnrVal = decryptedPayload.substring(rnrIdx + 4, rnrEnd);
      sos = (rnrVal == "1");
    }
    return true;
  }

  return false;
}

void sendTelemetryDirect(String deviceId, float lat, float lon, int pulse, bool sos) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure(); // Required for auto-rotating Railway certificates

  // FIX: Safe pattern for HTTPClient
  HTTPClient http;
  if (http.begin(client, backendURL)) {
    http.addHeader("Content-Type", "application/json");
    http.addHeader("x-device-key", DEVICE_API_KEY);

    // FIX: Bump to 512 for bigger payloads
    StaticJsonDocument<512> doc;
    doc["device_id"] = deviceId;
    doc["latitude"] = lat;
    doc["longitude"] = lon;
    doc["pulse"] = pulse;
    doc["battery"] = 100;
    doc["sos"] = sos;

    String requestBody;
    serializeJson(doc, requestBody);
    int httpResponseCode = http.POST(requestBody);
    
    if (httpResponseCode > 0) {
      Serial.println("POST code: " + String(httpResponseCode));
    } else {
      Serial.println("POST failed, error: " + http.errorToString(httpResponseCode));
    }
    http.end(); // Clear resources avoiding leaks
  } else {
    Serial.println("Unable to connect to backend URL");
  }
}

void connectWiFi() {
  Serial.print("Connecting WiFi");
  display.clearDisplay(); display.setCursor(0,0);
  display.println("Connecting WiFi..."); display.display();
  
  WiFi.begin(ssid, password);
  
  // FIX: Timeout for WiFi
  unsigned long start = millis();
  while(WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500); 
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_GREEN, HIGH); 
    display.clearDisplay(); display.setCursor(0,0);
    display.println("WIFI CONNECTED"); display.display();
    Serial.println("\nWiFi Connected");
  } else {
    digitalWrite(LED_GREEN, LOW); 
    display.clearDisplay(); display.setCursor(0,0);
    display.println("WIFI FAILED"); display.display();
    Serial.println("\nWiFi Failed Context");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(LED_YELLOW, HIGH); 
  digitalWrite(LED_RED, LOW);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Failed");
  }
  display.clearDisplay(); display.setTextColor(WHITE); display.setCursor(0,0);
  display.println("MIDWAY INIT"); display.display();

  connectWiFi();
  
  display.clearDisplay(); display.setCursor(0,0);
  display.println("BACKEND: Railway");
  display.println("Production Mode"); display.display();
  Serial.println("Backend: " + String(backendURL));
  delay(1000);

  SPI.begin(18, 19, 23, -1);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Failed");
  } else {
    LoRa.setSyncWord(0x34);
    Serial.println("LoRa Active");
  }
}

void loop() {
  // Reconnect WiFi if dropped
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_GREEN, LOW);
    connectWiFi();
  }

  // Blinking logic stabilizing
  if (millis() < sosBlinkUntil) {
    // Fast blink: 100ms ON, 100ms OFF (continuous toggle during 2s SOS mode)
    digitalWrite(LED_RED, (millis() % 200 < 100) ? HIGH : LOW);
  } else {
    // 100ms on, 900ms off - stable idle blink
    digitalWrite(LED_RED, (millis() % 1000 < 100) ? HIGH : LOW);
  }

  // FIX: Alive Heartbeat to show Live on Webpage even if no nodes
  if (millis() - lastHeartbeatTime > 30000) {
    lastHeartbeatTime = millis();
    if (WiFi.status() == WL_CONNECTED) {
       // Send an empty, safe payload to register activity for midway UI
       sendTelemetryDirect("midway_panel", 30.268423, 77.993792, 0, false);
    }
  }

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // FIX: String memory fragmentation prevention with char buffer
    char incomingBuf[256];
    int idx = 0;
    while(LoRa.available() && idx < 255) {
      incomingBuf[idx++] = (char)LoRa.read();
    }
    incomingBuf[idx] = '\0';
    String incoming = String(incomingBuf);
    
    // FIX: Safer Packet Parsing bounds
    int firstColon = incoming.indexOf(':');
    int secondColon = incoming.indexOf(':', firstColon + 1);
    int thirdColon = incoming.indexOf(':', secondColon + 1);
    int fourthColon = incoming.indexOf(':', thirdColon + 1);

    if (firstColon != -1 && secondColon != -1 && thirdColon != -1 && fourthColon != -1) {
      String msgId = incoming.substring(0, firstColon);
      String origId = incoming.substring(firstColon + 1, secondColon);
      String hexIV = incoming.substring(secondColon + 1, thirdColon);
      String hexCipher = incoming.substring(thirdColon + 1, fourthColon);
      String hexTag = incoming.substring(fourthColon + 1);

      if (!isNonceSeen(msgId)) {
        Serial.println("Midway Rcvd: " + origId);

        unsigned char* targetKey = NULL;
        if(origId == "Wristband") targetKey = key_wrist;
        else if(origId == "Node-A") targetKey = key_nodeA;
        else if(origId == "Node-B") targetKey = key_nodeB;
        else if(origId == "Node-C") targetKey = key_nodeC;

        if (targetKey != NULL) {
          String decrypted = decryptGCM(hexCipher, hexIV, hexTag, targetKey);
          if (decrypted.length() > 0) {
            int pipe1 = decrypted.indexOf('|');
            int pipe2 = decrypted.indexOf('|', pipe1 + 1);
            if (pipe1 > 0 && pipe2 > 0) {
              String innerNonce = decrypted.substring(0, pipe1);
              if (innerNonce == msgId) {
                if (!isNonceSeen(innerNonce)) {
                  addNonceSeen(innerNonce);
                  String actualPayload = decrypted.substring(pipe2 + 1);
                  Serial.println("DecPayload: " + actualPayload);

                  bool isSOS = (actualPayload.indexOf("RNR:1") >= 0 || actualPayload == "RNR");
                  if (isSOS) {
                      sosBlinkUntil = millis() + 2000;
                  }

                  display.clearDisplay(); display.setCursor(0,0);
                  if (isSOS) {
                      display.setTextSize(2);
                      display.println("SOS RECEIVED");
                      display.setTextSize(1);
                  } else {
                      display.println("PAYLOAD: " + origId);
                  }
                  display.display();

                  float lat=0, lon=0;
                  int pulse=0;
                  bool sos=false;
                  if (parseDecryptedPayload(origId, actualPayload, lat, lon, pulse, sos)) {
                     sendTelemetryDirect(origId, lat, lon, pulse, sos);
                  } else {
                     Serial.println("Unrecognized payload format");
                  }
                } else {
                  Serial.println("Replay Attack Blocked!");
                }
              } else {
                Serial.println("Nonce Mismatch!");
              }
            } else {
              Serial.println("Invalid payload structure inside GCM");
            }
          } else {
            Serial.println("Decryption failed / empty");
          }
        }
      }
    } else {
      Serial.println("Invalid packet format");
    }
  }
}
