#include "mbedtls/aes.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRa.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>


// ================= AES-256 Setup =================
const char *NODE_ID = "Node-A";

// 32-byte key for AES-256
unsigned char aes_key[32] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                             0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
                             0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                             0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};

// 16-byte initialization vector
unsigned char aes_iv[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

void encryptAndSend(String message) {
  int input_len = message.length();
  int padded_len = ((input_len / 16) + 1) * 16;
  unsigned char input[padded_len];
  uint8_t output[padded_len];

  // Clear and copy input
  memset(input, 0, padded_len);
  memcpy(input, message.c_str(), input_len);

  // PKCS7-like padding for compatibility
  uint8_t pad_val = padded_len - input_len;
  for (int i = input_len; i < padded_len; i++)
    input[i] = pad_val;

  unsigned char iv_copy[16];
  memcpy(iv_copy, aes_iv, 16);

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, aes_key, 256);
  mbedtls_aes_crypt_cbc(&aes,
                        MBEDTLS_AES_DECRYPT == 0 ? MBEDTLS_AES_ENCRYPT
                                                 : MBEDTLS_AES_ENCRYPT,
                        padded_len, iv_copy, (unsigned char *)input, output);
  mbedtls_aes_free(&aes);

  Serial.print("Sending Secure LoRa Packet (Size: ");
  Serial.print(padded_len);
  Serial.println(")");

  LoRa.beginPacket();
  LoRa.write(output, padded_len);
  LoRa.endPacket();
}

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
  display.print("NODE-A SECURE");
  display.println("AES-256 Enabled");
  display.display();

  pinMode(LORA_SS, OUTPUT);
  pinMode(RFID_SS, OUTPUT);
  digitalWrite(LORA_SS, HIGH);
  digitalWrite(RFID_SS, HIGH);

  SPI.begin(18, 19, 23, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa FAILED");
    while (1)
      ;
  }

  LoRa.setSyncWord(0x34);
  LoRa.setSpreadingFactor(7);
  LoRa.enableCrc();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("ENCRYPTED NODE A");
  display.println("READY");
  display.display();

  pinMode(SOS_BUTTON, INPUT_PULLUP);
  rfid.PCD_Init();
  Serial.println("RFID Reader Ready");
}

void loop() {
  if (digitalRead(SOS_BUTTON) == LOW) {
    Serial.println("SOS Triggered - Node-A");

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.println("SOS!");
    display.setTextSize(1);
    display.println("Sending encrypted...");
    display.display();

    encryptAndSend(NODE_ID);

    delay(2000);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("SECURE NODE READY");
    display.display();
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String userID = "user_1234";

    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("SECURE CHECKPOINT");
    display.display();

    String payload = "CP:" + userID + ":" + String(NODE_ID);
    encryptAndSend(payload);

    Serial.println("Checkpoint Transmitted Securely");

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    delay(2000);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("SECURE NODE READY");
    display.display();
  }
}