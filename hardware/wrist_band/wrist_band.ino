#include "mbedtls/aes.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRa.h>
#include <SPI.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// ================= AES-256 Setup =================
const char *DEVICE_ID = "WristBand-01";

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

// ================= OLED Pins =================
#define OLED_SDA 4
#define OLED_SCL 15
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= SOS & GPS Pins =================
#define SOS_BUTTON 13
#define GPS_RX 16
#define GPS_TX 17 // ESP TX goes to GPS RX. If GPS TX goes to ESP 16, then ESP RX is 16.

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

float currentLat = 0.0;
float currentLon = 0.0;
bool hasFix = false;

void encryptAndSend(String message) {
  int input_len = message.length();
  int padded_len = ((input_len / 16) + 1) * 16;
  unsigned char input[padded_len];
  uint8_t output[padded_len];

  memset(input, 0, padded_len);
  memcpy(input, message.c_str(), input_len);

  unsigned char iv_copy[16];
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

  Serial.println("Packet Sent!");
}

void setup() {
  Serial.begin(115200);
  
  // Initialize Serial2 for GPS
  // GPS TX -> ESP32 RX (16), GPS RX -> ESP32 TX (17)
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  
  pinMode(SOS_BUTTON, INPUT_PULLUP);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED INIT FAILED");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("WristBand INIT");
  display.display();

  SPI.begin(18, 19, 23, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa FAILED");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("LoRa FAILED");
    display.display();
    while (1);
  }

  LoRa.setSyncWord(0x34);
  LoRa.setSpreadingFactor(7);
  LoRa.enableCrc();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WristBand READY");
  display.println("GPS & LoRa OK");
  display.display();
}

void loop() {
  // Parse GPS
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
  
  if (gps.location.isUpdated()) {
    currentLat = gps.location.lat();
    currentLon = gps.location.lng();
    hasFix = true;
  }

  // Handle SOS
  if (digitalRead(SOS_BUTTON) == LOW) {
    Serial.println("SOS Pressed!");

    String payload = "{";
    payload += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
    if (hasFix) {
      payload += "\"latitude\":" + String(currentLat, 6) + ",";
      payload += "\"longitude\":" + String(currentLon, 6) + ",";
    } else {
      // Send 0.0 if no GPS fix yet, or a fallback coordinate
      payload += "\"latitude\":0.000000,";
      payload += "\"longitude\":0.000000,";
    }
    payload += "\"sos\":true";
    payload += "}";

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("SOS SENT!");
    display.setTextSize(1);
    display.setCursor(0, 30);
    if (hasFix) {
      display.print("Lat: "); display.println(currentLat, 4);
      display.print("Lon: "); display.println(currentLon, 4);
    } else {
      display.println("No GPS Fix yet!");
    }
    display.display();

    encryptAndSend(payload);
    
    // Debounce & spam prevention
    delay(3000);
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WristBand READY");
    if (hasFix) display.println("GPS Linked");
    else display.println("Waiting GPS");
    display.display();
  }
}
