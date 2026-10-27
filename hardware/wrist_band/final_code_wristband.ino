#include <Adafruit_GFX.h>
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

  String payload = "BPM:" + String(bpm) + "|LAT:" + latStr + "|LON:" + lonStr + "|RNR:" + (sosActive ? "1" : "0");
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
  SPI.begin(18, 19, 23, -1);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) while(1);
  LoRa.setSyncWord(0x34);

  pinMode(SOS_BUTTON, INPUT_PULLUP);
  randomSeed(analogRead(0));
}

void readPulse() {
    int sensorValue = analogRead(PULSE_PIN);
    if (sensorValue < 100) bpm = 0; 
    else bpm = map(sensorValue, 0, 4095, 50, 150); 
}

void loop() {
  while (Serial1.available() > 0) gps.encode(Serial1.read());

  unsigned long currentMillis = millis();

  if (currentMillis - lastPulseRead >= 1000) {
    lastPulseRead = currentMillis;
    readPulse();

    bool physicalSos = (digitalRead(SOS_BUTTON) == LOW);
    bool autoSos = (bpm > 0 && (bpm < 40 || bpm > 120)); 

    if (physicalSos || autoSos) {
      if (!sosActive) {
          sosActive = true;
          sendWristbandData(); 
      }
    } else {
      sosActive = false;
    }
  }

  if (currentMillis - lastSend >= 5000) {
    lastSend = currentMillis;
    sendWristbandData();
  }
}
