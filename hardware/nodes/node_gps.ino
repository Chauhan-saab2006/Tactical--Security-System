#include "mbedtls/aes.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRa.h>
#include <SPI.h>
#include <Wire.h>

const char *NODE_ID = "Node-B"; // can be modified depending on the node

unsigned char aes_key[32] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                             0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
                             0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                             0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};

unsigned char aes_iv[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

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

void encryptAndSend(String message) {
  int input_len = message.length();
  int padded_len = ((input_len / 16) + 1) * 16;
  unsigned char input[padded_len];
  uint8_t output[padded_len];

  memset(input, 0, padded_len);
  memcpy(input, message.c_str(), input_len);

  unsigned char iv_copy[16]; // mbedtls modifies the IV, need a copy
  memcpy(iv_copy, aes_iv, 16);

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, aes_key, 256);
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded_len, iv_copy,
                        (unsigned char *)input, output);
  mbedtls_aes_free(&aes);

  LoRa.beginPacket();
  LoRa.write(output, padded_len);
  LoRa.endPacket();
}

void decryptAndShow(uint8_t* raw_input, int length) {
  unsigned char iv_copy[16];
  memcpy(iv_copy, aes_iv, 16);
  
  char decrypted[256];
  memset(decrypted, 0, sizeof(decrypted));

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_dec(&aes, aes_key, 256);
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, length, iv_copy, raw_input, (unsigned char*)decrypted);
  mbedtls_aes_free(&aes);

  String decStr = String(decrypted);
  Serial.println("Decrypted: " + decStr);
  
  if (decStr.indexOf("\"sos\":true") > 0 || decStr.indexOf("\"sos\": true") > 0 || decStr.indexOf("SOS") >= 0) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("SOS ALERT!");
    
    display.setTextSize(1);
    int latIdx = decStr.indexOf("\"latitude\":");
    int lonIdx = decStr.indexOf("\"longitude\":");
    if (latIdx > 0 && lonIdx > 0) {
      float lat = decStr.substring(latIdx + 11, decStr.indexOf(",", latIdx)).toFloat();
      float lon = decStr.substring(lonIdx + 12, decStr.indexOf(",", lonIdx)).toFloat();
      display.setCursor(0, 30);
      display.print("Lat: "); display.println(lat, 5);
      display.print("Lon: "); display.println(lon, 5);
    } else {
      display.setCursor(0, 30);
      display.println(decStr); // raw text like Node-B
    }
    display.display();
    delay(5000);
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("SECURE NODE READY");
    display.display();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED INIT FAILED");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(NODE_ID);
  display.println(" SECURE");
  display.println("AES-256 Enabled");
  display.display();

  SPI.begin(18, 19, 23, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa FAILED");
    while (1);
  }

  LoRa.setSyncWord(0x34);
  LoRa.setSpreadingFactor(7);
  LoRa.enableCrc();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("ENCRYPTED NODE");
  display.println("READY TO RX/TX (GPS)");
  display.display();

  pinMode(SOS_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(SOS_BUTTON) == LOW) {
    Serial.println("Sending SECURE SOS from Node");

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.println("SOS!");
    display.setTextSize(1);
    display.println("Transmitting AES-256...");
    display.display();

    encryptAndSend(NODE_ID);

    delay(2000);

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("SECURE NODE READY");
    display.display();
  }
  
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    uint8_t buffer[256];
    int len = 0;
    while (LoRa.available() && len < 256) {
      buffer[len++] = (uint8_t)LoRa.read();
    }
    if (len % 16 == 0) { // AES blocks are multiples of 16
      decryptAndShow(buffer, len);
    } else {
      Serial.println("Invalid packet size, ignoring.");
    }
  }
}
