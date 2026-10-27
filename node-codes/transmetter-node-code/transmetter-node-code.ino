#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
//                        LORA PINS
// ============================================================

#define LORA_SS     5
#define LORA_RST    14
#define LORA_DIO0   26

#define LORA_SCK    18
#define LORA_MISO   19
#define LORA_MOSI   23

// ============================================================
//                        OLED PINS
// ============================================================

#define OLED_SDA    21
#define OLED_SCL    22

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
#define OLED_ADDRESS 0x3C

// ============================================================
//                        BUZZER
// ============================================================

#define BUZZER_PIN  27

// ============================================================
//                        LORA SETTINGS
// ============================================================

#define LORA_FREQUENCY 433E6

#define LORA_SF 7
#define LORA_BW 125E3
#define LORA_CR 5

// ============================================================
//                        OLED
// ============================================================

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// ============================================================
//                        READY SCREEN
// ============================================================

void showReady()
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("SOS RECEIVER");

  display.setCursor(0, 20);
  display.println("LoRa: READY");

  display.setCursor(0, 40);
  display.println("WAITING FOR SOS...");

  display.display();
}

// ============================================================
//                        SOS SCREEN
// ============================================================

void showSOS()
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);

  display.setCursor(35, 5);
  display.println("SOS!");

  display.setTextSize(1);

  display.setCursor(20, 35);
  display.println("EMERGENCY ALERT");

  display.display();
}

// ============================================================
//                        SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);

  // ----------------------------------------------------------
  // BUZZER
  // ----------------------------------------------------------

  pinMode(BUZZER_PIN, OUTPUT);

  // Buzzer OFF initially
  digitalWrite(BUZZER_PIN, LOW);

  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS))
  {
    Serial.println("OLED ERROR!");

    while (true)
    {
      delay(1000);
    }
  }

  showReady();

  // ----------------------------------------------------------
  // SPI
  // ----------------------------------------------------------

  SPI.begin(
    LORA_SCK,
    LORA_MISO,
    LORA_MOSI,
    LORA_SS
  );

  // ----------------------------------------------------------
  // LoRa pins
  // ----------------------------------------------------------

  LoRa.setPins(
    LORA_SS,
    LORA_RST,
    LORA_DIO0
  );

  // ----------------------------------------------------------
  // Start LoRa
  // ----------------------------------------------------------

  Serial.println("Starting LoRa...");

  if (!LoRa.begin(LORA_FREQUENCY))
  {
    Serial.println("LORA FAILED!");

    while (true)
    {
      delay(1000);
    }
  }

  // ----------------------------------------------------------
  // SAME SETTINGS AS TRANSMITTER
  // ----------------------------------------------------------

  LoRa.setSpreadingFactor(LORA_SF);

  LoRa.setSignalBandwidth(LORA_BW);

  LoRa.setCodingRate4(LORA_CR);

  Serial.println("LoRa READY");
  Serial.println("Waiting for SOS...");

  // Start receiving
  LoRa.receive();
}

// ============================================================
//                        LOOP
// ============================================================

void loop()
{
  int packetSize = LoRa.parsePacket();

  if (packetSize > 0)
  {
    String message = "";

    while (LoRa.available())
    {
      message += (char)LoRa.read();
    }

    Serial.print("Received: ");
    Serial.println(message);

    // ========================================================
    //                    SOS RECEIVED
    // ========================================================

    if (message == "SOS")
    {
      Serial.println("!!! SOS RECEIVED !!!");

      // ------------------------------------------------------
      // BUZZER ON
      // ------------------------------------------------------

      digitalWrite(BUZZER_PIN, HIGH);

      // OLED SOS
      showSOS();

      // Buzzer ON for 2 seconds
      delay(2000);

      // Buzzer OFF
      digitalWrite(BUZZER_PIN, LOW);

      Serial.println("Resetting receiver...");

      delay(500);

      // Reset ESP32
      ESP.restart();
    }

    // If not SOS, continue listening
    LoRa.receive();
  }

  delay(5);
}