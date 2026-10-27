#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <time.h>

// ================= WIFI SETTINGS =================
const char *ssid = "Veer";
const char *password = "veer1708";

// Backend Production URL (Railway)
const char *backendURL = "https://lifeline-production-1041.up.railway.app/api/v1/telemetry";
const char *deviceApiKey = "antigravity_secret_123";

// ================= OLED SETTINGS =================
#define OLED_SDA 4
#define OLED_SCL 15
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= HARDWARE PINS =================
#define SOS_BUTTON 32       // SOS trigger button
#define PULSE_SENSOR_PIN 34 // Pulse sensor digital output

// ================= CONSTANTS =================
const char *DEVICE_ID = "Wristband-01";
const float FIXED_LAT = 29.211000;
const float FIXED_LNG = 77.016500;

// ================= STATE VARIABLES =================
bool sosActive = false;
unsigned long lastTelemetryTime = 0;
const int telemetryInterval = 1000; // 1 second

// Pulse Calculation Variables
volatile unsigned long lastPulseTime = 0;
volatile int pulseCount = 0;
volatile int currentBPM = 0;
unsigned long lastBPMCalcTime = 0;

// ================= TIME SETTINGS =================
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800; // New Delhi (GMT +5:30)
const int daylightOffset_sec = 0;

// Forward Declarations
void IRAM_ATTR handlePulse();
void sendTelemetry(bool sos);
void updateDisplay();
void calculateBPM();
void showSOSSent();

void setup() {
  Serial.begin(115200);

  // OLED Init
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED INIT FAILED");
    for (;;)
      ;
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("LoRa success"); // User requested this specifically
  display.display();
  delay(1000);

  // WiFi Connect
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  // Sync Time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  pinMode(SOS_BUTTON, INPUT_PULLUP);
  pinMode(PULSE_SENSOR_PIN, INPUT); // Digital input from pulse sensor

  attachInterrupt(digitalPinToInterrupt(PULSE_SENSOR_PIN), handlePulse, RISING);

  display.clearDisplay();
  display.println("SYSTEM READY");
  display.display();
  delay(1000);
}

void IRAM_ATTR handlePulse() {
  unsigned long now = millis();
  if (now - lastPulseTime > 200) { // Simple debounce / limit to 300bpm max
    pulseCount++;
    lastPulseTime = now;
  }
}

void loop() {
  // 1. SOS Button Check
  if (digitalRead(SOS_BUTTON) == LOW) {
    sosActive = true;
    Serial.println("SOS TRIGGERED!");
    sendTelemetry(true);
    showSOSSent();
    delay(2000); // Debounce and show message
    sosActive = false;
  }

  // 2. Periodic Telemetry (Every 1 second)
  if (millis() - lastTelemetryTime >= telemetryInterval) {
    lastTelemetryTime = millis();
    sendTelemetry(sosActive);
    updateDisplay();
  }
}

void calculateBPM() {
  unsigned long now = millis();
  if (now - lastBPMCalcTime >= 10000) { // Every 10 seconds
    currentBPM = pulseCount * 6;        // Rough estimate (pulses in 10s * 6)
    pulseCount = 0;
    lastBPMCalcTime = now;
    Serial.print("CALCULATED BPM: ");
    Serial.println(currentBPM);
  }
}

int getRealPulse() {
  // If sensor is not active (long time since last pulse), return 0
  if (millis() - lastPulseTime > 2000) {
    return 0;
  }
  return currentBPM;
}

void sendTelemetry(bool sos) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, backendURL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Device-Key", deviceApiKey);

    int pulse = getRealPulse();

    String jsonPayload = "{";
    jsonPayload += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
    jsonPayload += "\"latitude\":" + String(FIXED_LAT, 6) + ",";
    jsonPayload += "\"longitude\":" + String(FIXED_LNG, 6) + ",";
    jsonPayload += "\"pulse\":" + String(pulse) + ",";
    jsonPayload += "\"sos\":" + String(sos ? "true" : "false");
    jsonPayload += "}";

    int httpCode = http.POST(jsonPayload);
    http.end();
  }
}

void updateDisplay() {
  display.clearDisplay();

  // Show Real Time
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    display.setCursor(0, 0);
    display.println("Time Error");
  } else {
    display.setCursor(0, 0);
    display.setTextSize(1);
    char timeStr[10];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    display.print("TIME: ");
    display.println(timeStr);
  }

  // Pulse & Wearing status
  calculateBPM();
  int pulse = getRealPulse();
  display.setCursor(0, 20);
  if (pulse == 0) {
    display.setTextSize(1);
    display.println("STATUS: NO PULSE");
  } else {
    display.print("PULSE: ");
    display.print(pulse);
    display.println(" BPM");
  }

  // SOS Status
  display.setCursor(0, 40);
  display.print("SOS: ");
  display.println(sosActive ? "ACTIVE" : "READY");

  display.display();
}

void showSOSSent() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 25);
  display.println("SOS SENT");
  display.display();
}
