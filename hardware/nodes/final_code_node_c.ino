#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRa.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>
#include "mbedtls/gcm.h"
#include <esp_system.h>

// ================= DEVICE & MESH CONFIG =================
const char *NODE_ID = "Node-C";
unsigned char aes_key[32] = {'N','O','D','E','C','K','E','Y','A','E','S','1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F','1','2','3','4','5'};
// Binary: 01001110 01001111 01000100 01000101 01000011 01001011 01000101 01011001 01000001 01000101 01010011 00110001 00110010 00110011 00110100 00110101 00110110 00110111 00111000 00111001 00110000 01000001 01000010 01000011 01000100 01000101 01000110 00110001 00110010 00110011 00110100 00110101

// Mesh loop prevention
String recentMsgIDs[15];
int msgIdx = 0;

// ================= PINS =================
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26
#define OLED_SDA 4
#define OLED_SCL 15
Adafruit_SSD1306 display(128, 64, &Wire, -1);
#define SOS_BUTTON 32
#define RFID_SS 21
#define RFID_RST 27
MFRC522 rfid(RFID_SS, RFID_RST);

bool isMsgSeen(String mid) {
  for(int i=0; i<15; i++) {
    if(recentMsgIDs[i] == mid) return true;
  }
  return false;
}

bool isButtonPressed() {
  if (digitalRead(SOS_BUTTON) != LOW) return false;
  delay(30);
  return digitalRead(SOS_BUTTON) == LOW;
}
void addMsgSeen(String mid) {
  recentMsgIDs[msgIdx] = mid;
  msgIdx = (msgIdx + 1) % 15;
}

void forwardMeshPacket(String packet) {
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();
}

String byteToHex(uint8_t b) {
  String hexStr = String(b, HEX);
  if(hexStr.length() < 2) hexStr = "0" + hexStr;
  return hexStr;
}

String bytesToHex(uint8_t* bytes, size_t len) {
  String hexStr = "";
  for(size_t i = 0; i < len; i++) {
    hexStr += byteToHex(bytes[i]);
  }
  return hexStr;
}

void generateOrigPacketAndSend(String payload) {
  uint8_t nonceBytes[8];
  esp_fill_random(nonceBytes, sizeof(nonceBytes));
  String nonceStr = bytesToHex(nonceBytes, sizeof(nonceBytes));
  nonceStr.toUpperCase();
  
  addMsgSeen(nonceStr);

  uint8_t ivBytes[12];
  esp_fill_random(ivBytes, sizeof(ivBytes));
  String ivStr = bytesToHex(ivBytes, sizeof(ivBytes));
  ivStr.toUpperCase();

  String plaintext = nonceStr + "|" + String(millis()) + "|" + payload;
  
  int input_len = plaintext.length();
  unsigned char* input = (unsigned char*)plaintext.c_str();
  uint8_t output[input_len];
  uint8_t tag[16];

  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);
  mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, aes_key, 256);
  
  mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, input_len,
                            ivBytes, sizeof(ivBytes),
                            NULL, 0,
                            input, output,
                            sizeof(tag), tag);
  mbedtls_gcm_free(&gcm);

  String cipherStr = bytesToHex(output, input_len);
  cipherStr.toUpperCase();
  String tagStr = bytesToHex(tag, sizeof(tag));
  tagStr.toUpperCase();

  String packet = nonceStr + ":" + String(NODE_ID) + ":" + ivStr + ":" + cipherStr + ":" + tagStr;
  Serial.println("Originated: " + packet);
  
  display.clearDisplay(); display.setCursor(0,0);
  display.print("TX: " + payload); display.display();
  
  forwardMeshPacket(packet);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) while(1);
  display.clearDisplay(); display.setTextColor(WHITE); display.setCursor(0,0);
  display.print("NODE: "); display.println(NODE_ID); display.display();

  pinMode(LORA_SS, OUTPUT); pinMode(RFID_SS, OUTPUT);
  digitalWrite(LORA_SS, HIGH); digitalWrite(RFID_SS, HIGH);
  SPI.begin(18, 19, 23, -1);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
      display.println("LORA FAILED"); display.display();
      while(1);
  }
  LoRa.setSyncWord(0x34);
  
  pinMode(SOS_BUTTON, INPUT_PULLUP);
  rfid.PCD_Init();
  randomSeed(analogRead(0));
  display.println("Mesh Ready."); display.display();
}

void loop() {
  if (isButtonPressed()) {
    generateOrigPacketAndSend("RNR:1");
    delay(2000);
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    generateOrigPacketAndSend("CP:user_1:Node-C");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    delay(2000);
  }

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = "";
    while(LoRa.available()) incoming += (char)LoRa.read();
    
    int firstColon = incoming.indexOf(':');
    if(firstColon > 0) {
      String msgId = incoming.substring(0, firstColon);
      if(!isMsgSeen(msgId)) {
        addMsgSeen(msgId);
        Serial.println("Forwarding: " + incoming);
        display.clearDisplay(); display.setCursor(0,0); display.println("FWD: " + msgId); display.display();
        // PASS-THROUGH NO DECRYPTION
        forwardMeshPacket(incoming); 
      }
    }
  }
}
