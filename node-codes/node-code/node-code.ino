#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// ============================================================
// LIFELINK SOS RECEIVER
// ESP32 + I2C OLED + LoRa + WiFi + Firebase + Buzzer + LEDs
// ============================================================

// ---------------- OLED ----------------

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1
);

bool oledReady = false;

// ---------------- WIFI ----------------

const char* WIFI_SSID = "Galaxy A17 5G 406A";
const char* WIFI_PASS = "12345678";

// ---------------- FIREBASE ----------------

const char* FIREBASE_URL =
  "YOUR_FIREBASE_PROJECT_URL"; // Replace with your Firebase project URL

const char* API_KEY =
  "YOUR_FIREBASE_API_KEY"; // Replace with your Firebase API key

// ---------------- COORDINATES ----------------

const int COORD_COUNT = 10;

const double LATITUDES[COORD_COUNT] = {
  28.4611576,
  28.4610946,
  28.4614150,
  28.4614662,
  28.4607134,
  28.4603762,
  28.4613253,
  28.4598348,
  28.4612608,
  28.4614223
};

const double LONGITUDES[COORD_COUNT] = {
  77.4898174,
  77.4902456,
  77.4900782,
  77.4893192,
  77.4892917,
  77.4895395,
  77.4892739,
  77.4903619,
  77.4891180,
  77.4897359
};

// IMPORTANT: The original longitude list has only 9 values
// for 10 coordinates. Add the verified 10th longitude.

// ---------------- PREFERENCES ----------------

Preferences prefs;
int coordIndex = 0;

// ---------------- LORA PINS ----------------

#define LORA_SS     5
#define LORA_RST    14
#define LORA_DIO0   32

#define LORA_SCK    18
#define LORA_MISO   19
#define LORA_MOSI   23

// ---------------- TRAFFIC LIGHT LEDS ----------------

#define LED_RED_PIN     27
#define LED_YELLOW_PIN  26
#define LED_GREEN_PIN   25

// ---------------- BUZZER ----------------

#define BUZZER_PIN  33
#define BUZZER_FREQ 2000

// SOS indicator state
bool sosLedActive = false;

// ============================================================
// STATUS LED CONTROL
// ============================================================

void updateStatusLeds() {

  // Green: ESP32 firmware is running
  digitalWrite(LED_GREEN_PIN, HIGH);

  // Yellow: WiFi connected
  digitalWrite(
    LED_YELLOW_PIN,
    WiFi.status() == WL_CONNECTED ? HIGH : LOW
  );

  // Red: SOS emergency received
  digitalWrite(
    LED_RED_PIN,
    sosLedActive ? HIGH : LOW
  );
}

// ---------------- LORA SETTINGS ----------------

#define LORA_FREQUENCY 433E6
#define LORA_SF        7
#define LORA_BW        125E3
#define LORA_CR        5

// ---------------- RADAR ANIMATION ----------------

int radarAngle = 0;
int radarFrame = 0;
unsigned long lastRadarFrame = 0;

// ============================================================
// OLED STATUS DISPLAY
// ============================================================

void showStatus(
  const char* title,
  const char* detail = ""
) {
  Serial.println(title);

  if (!oledReady) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print("LIFELINK / RECEIVER");

  display.drawFastHLine(
    0, 12, 128, SSD1306_WHITE
  );

  display.setTextSize(2);
  display.setCursor(2, 22);
  display.println(title);

  display.setTextSize(1);
  display.setCursor(2, 49);
  display.println(detail);

  display.display();
}

// ============================================================
// WIFI CONNECTION PROGRESS DISPLAY
// ============================================================

void showWiFiProgress(int attempt) {

  if (!oledReady) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(2, 2);
  display.print("LIFELINK / NETWORK");

  display.drawFastHLine(
    0, 12, 128, SSD1306_WHITE
  );

  display.setTextSize(2);
  display.setCursor(2, 20);
  display.print("CONNECTING");

  display.setTextSize(1);
  display.setCursor(2, 40);
  display.print("WiFi attempt: ");
  display.print(attempt);

  int position = (attempt % 6) * 18;

  display.drawRect(
    2, 53, 124, 8, SSD1306_WHITE
  );

  display.fillRect(
    4 + position, 55, 16, 4, SSD1306_WHITE
  );

  display.display();
}

// ============================================================
// WIFI CONNECTION
// ============================================================

void connectWiFiFirst() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempt = 0;

  while (WiFi.status() != WL_CONNECTED) {

    attempt++;

    showWiFiProgress(attempt);
    updateStatusLeds();

    Serial.print("Connecting to WiFi, attempt ");
    Serial.println(attempt);

    unsigned long start = millis();

    while (
      WiFi.status() != WL_CONNECTED &&
      millis() - start < 5000
    ) {
      updateStatusLeds();
      delay(100);
    }

    if (WiFi.status() != WL_CONNECTED) {

      showStatus("RETRY WIFI", "Check hotspot");

      delay(1000);

      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
  }

  updateStatusLeds();

  Serial.println("WiFi connected!");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  showStatus("WIFI CONNECTED", "Network ready");

  delay(1200);

  if (oledReady) {

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(2, 3);
    display.println("NETWORK CONNECTED");

    display.drawFastHLine(
      0, 14, 128, SSD1306_WHITE
    );

    display.setCursor(2, 25);
    display.println("ESP32 IP:");

    display.setCursor(2, 40);
    display.println(WiFi.localIP().toString());

    display.display();

    delay(1800);
  }
}

// ============================================================
// RADAR ANIMATION
// ============================================================

void drawRadarScreen() {

  if (!oledReady) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(2, 2);
  display.print("LIFELINK / ACTIVE");

  display.drawFastHLine(
    0, 12, 128, SSD1306_WHITE
  );

  const int cx = 33;
  const int cy = 39;
  const int r = 22;

  display.drawCircle(cx, cy, r, SSD1306_WHITE);
  display.drawCircle(cx, cy, 14, SSD1306_WHITE);
  display.drawCircle(cx, cy, 6, SSD1306_WHITE);

  display.drawFastHLine(
    cx - r, cy, r * 2 + 1, SSD1306_WHITE
  );

  display.drawFastVLine(
    cx, cy - r, r * 2 + 1, SSD1306_WHITE
  );

  float a = radarAngle * PI / 180.0;

  int ex = cx + (int)(cos(a) * r);
  int ey = cy + (int)(sin(a) * r);

  display.drawLine(cx, cy, ex, ey, SSD1306_WHITE);
  display.fillCircle(cx, cy, 2, SSD1306_WHITE);

  int tx = cx + 10;
  int ty = cy - 10;
  int pulse = (radarFrame / 3) % 4;

  display.drawCircle(
    tx, ty, 2 + pulse, SSD1306_WHITE
  );

  display.fillCircle(tx, ty, 1, SSD1306_WHITE);

  display.setCursor(64, 20);
  display.print("RADAR");

  display.setCursor(64, 31);
  display.print("SCANNING");

  display.setCursor(64, 42);
  display.print("SOS READY");

  int bars = (radarFrame / 2) % 6;

  for (int i = 0; i < 5; i++) {

    int h = 3 + i * 2;
    int x = 65 + i * 11;
    int y = 61 - h;

    if (i < bars) {

      display.fillRect(
        x, y, 5, h, SSD1306_WHITE
      );

    } else {

      display.drawRect(
        x, y, 5, h, SSD1306_WHITE
      );
    }
  }

  display.display();

  radarAngle = (radarAngle + 12) % 360;
  radarFrame++;
}

// ============================================================
// SOS EMERGENCY ANIMATION
// ============================================================

void showEmergencyAnimation() {

  showStatus("SOS RECEIVED", "Emergency detected!");

  delay(500);

  for (int frame = 0; frame < 8; frame++) {

    if (oledReady) {

      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);

      display.setTextSize(1);
      display.setCursor(2, 2);
      display.print("!!! EMERGENCY !!!");

      display.drawFastHLine(
        0, 12, 128, SSD1306_WHITE
      );

      int radius = 5 + (frame % 4) * 4;

      display.drawCircle(
        28, 32, radius, SSD1306_WHITE
      );

      display.drawCircle(
        28, 32, 4, SSD1306_WHITE
      );

      display.drawTriangle(
        28, 17,
        15, 43,
        41, 43,
        SSD1306_WHITE
      );

      display.setCursor(26, 27);
      display.print("!");

      display.setTextSize(2);
      display.setCursor(48, 21);
      display.print("SOS!");

      display.setTextSize(1);
      display.setCursor(46, 42);
      display.print("HELP NEEDED");

      display.setCursor(46, 53);
      display.print("ALERT ACTIVE");

      display.display();
    }

    // Buzzer alert
    ledcWriteTone(BUZZER_PIN, BUZZER_FREQ);
    delay(180);

    ledcWriteTone(BUZZER_PIN, 0);
    delay(70);
  }
}

// ============================================================
// FIREBASE SOS UPLOAD
// ============================================================

bool sendSOStoFirebase() {

  if (WiFi.status() != WL_CONNECTED) {

    showStatus("NO WIFI", "Reconnecting...");

    WiFi.reconnect();

    unsigned long start = millis();

    while (
      WiFi.status() != WL_CONNECTED &&
      millis() - start < 10000
    ) {
      updateStatusLeds();
      delay(200);
    }

    if (WiFi.status() != WL_CONNECTED) {

      showStatus("UPLOAD FAILED", "WiFi unavailable");

      return false;
    }
  }

  updateStatusLeds();

  showStatus("UPLOADING", "Sending SOS to Firebase");

  if (
    coordIndex < 0 ||
    coordIndex >= COORD_COUNT
  ) {
    coordIndex = 0;
  }

  double lat = LATITUDES[coordIndex];
  double lon = LONGITUDES[coordIndex];

  HTTPClient http;

  String url = String(FIREBASE_URL) +
               "/LifeLink-SosAlert.json?key=" +
               API_KEY;

  http.setTimeout(10000);

  if (!http.begin(url)) {

    showStatus("UPLOAD FAILED", "HTTP setup error");

    return false;
  }

  http.addHeader(
    "Content-Type", "application/json"
  );

  String json = "{";

  json += "\"node\":\"Base-1\",";
  json += "\"status\":\"SOS\",";
  json += "\"message\":\"Emergency Alert\",";
  json += "\"latitude\":" + String(lat, 6) + ",";
  json += "\"longitude\":" + String(lon, 6) + ",";
  json += "\"timestamp\":{\".sv\":\"timestamp\"}";

  json += "}";

  Serial.println("Posting SOS to Firebase...");

  int code = http.POST(json);

  Serial.print("Firebase HTTP code: ");
  Serial.println(code);

  bool success = (code >= 200 && code < 300);

  if (success) {

    Serial.println("Firebase upload successful");

    Serial.print("Coordinates: ");
    Serial.print(lat, 6);
    Serial.print(", ");
    Serial.println(lon, 6);

    coordIndex = (coordIndex + 1) % COORD_COUNT;

    prefs.putInt("index", coordIndex);

  } else {

    Serial.println("Firebase upload failed");

    if (code > 0) {

      Serial.println(http.getString());

    } else {

      Serial.println(http.errorToString(code));
    }
  }

  http.end();

  return success;
}

// ============================================================
// SHOW FIREBASE RESULT
// ============================================================

void showUploadResult(bool success) {

  if (!oledReady) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print("LIFELINK / RESULT");

  display.drawFastHLine(
    0, 12, 128, SSD1306_WHITE
  );

  if (success) {

    display.drawCircle(
      14, 31, 9, SSD1306_WHITE
    );

    display.drawLine(
      9, 31, 13, 35, SSD1306_WHITE
    );

    display.drawLine(
      13, 35, 20, 26, SSD1306_WHITE
    );

    display.setTextSize(2);
    display.setCursor(29, 22);
    display.print("SENT OK");

    display.setTextSize(1);
    display.setCursor(4, 49);
    display.print("Firebase updated");

  } else {

    display.setTextSize(2);
    display.setCursor(2, 23);
    display.print("FAILED");

    display.setTextSize(1);
    display.setCursor(2, 49);
    display.print("Check WiFi / DB");
  }

  display.display();

  delay(1800);
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  // Initialize the status LEDs
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);

  sosLedActive = false;

  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_YELLOW_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, HIGH);

  Serial.begin(115200);

  delay(500);

  Serial.println();
  Serial.println("==============================");
  Serial.println("LIFELINK SOS RECEIVER");
  Serial.println("==============================");

  // Initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  if (
    display.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDR
    )
  ) {

    oledReady = true;

    showStatus(
      "LIFELINK",
      "Booting receiver..."
    );

  } else {

    Serial.println("OLED not detected!");
  }

  // Initialize preferences
  prefs.begin("lifelink", false);

  coordIndex = prefs.getInt("index", 0);

  if (
    coordIndex < 0 ||
    coordIndex >= COORD_COUNT
  ) {

    coordIndex = 0;

    prefs.putInt("index", 0);
  }

  // Initialize buzzer (ESP32 Arduino core 3.x)
  if (
    !ledcAttach(
      BUZZER_PIN,
      BUZZER_FREQ,
      8
    )
  ) {

    Serial.println("Buzzer PWM setup failed");
  }

  ledcWriteTone(BUZZER_PIN, 0);

  // Connect WiFi first
  connectWiFiFirst();

  updateStatusLeds();

  // Initialize LoRa
  showStatus(
    "LORA START",
    "Initializing radio..."
  );

  SPI.begin(
    LORA_SCK,
    LORA_MISO,
    LORA_MOSI,
    LORA_SS
  );

  LoRa.setPins(
    LORA_SS,
    LORA_RST,
    LORA_DIO0
  );

  if (!LoRa.begin(LORA_FREQUENCY)) {

    Serial.println("LoRa initialization failed!");

    showStatus(
      "LORA FAILED",
      "Check radio wiring"
    );

    while (true) {
      updateStatusLeds();
      delay(1000);
    }
  }

  LoRa.setSpreadingFactor(LORA_SF);

  LoRa.setSignalBandwidth(LORA_BW);

  LoRa.setCodingRate4(LORA_CR);

  showStatus(
    "LORA READY",
    "Radio initialized"
  );

  delay(1000);

  // Begin receiving
  LoRa.receive();

  radarAngle = 0;
  radarFrame = 0;
  lastRadarFrame = 0;

  showStatus(
    "SYSTEM READY",
    "Starting radar..."
  );

  delay(500);

  updateStatusLeds();

  Serial.println(
    "Receiver ready. Waiting for SOS."
  );
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {

  // Update WiFi and status LEDs
  updateStatusLeds();

  // Animate radar
  if (
    millis() - lastRadarFrame >= 90
  ) {

    lastRadarFrame = millis();

    drawRadarScreen();
  }

  // Check for LoRa packets
  int packetSize = LoRa.parsePacket();

  if (packetSize <= 0) {

    delay(2);

    return;
  }

  String message = "";

  while (LoRa.available()) {

    message += (char)LoRa.read();
  }

  message.trim();

  Serial.println();

  Serial.print("Received LoRa packet: ");
  Serial.println(message);

  // Ignore packets that are not SOS
  if (message != "SOS") {

    Serial.println(
      "Packet ignored - not SOS"
    );

    LoRa.receive();

    return;
  }

  // Turn on red LED when SOS is received
  sosLedActive = true;

  updateStatusLeds();

  Serial.println("SOS RECEIVED!");

  // Emergency animation and buzzer
  showEmergencyAnimation();

  // Upload SOS to Firebase
  bool uploaded = sendSOStoFirebase();

  // Display upload result
  showUploadResult(uploaded);

  // Turn off buzzer
  ledcWriteTone(BUZZER_PIN, 0);

  // Clear red SOS LED after processing
  sosLedActive = false;

  updateStatusLeds();

  // Reset radar animation
  radarAngle = 0;
  radarFrame = 0;
  lastRadarFrame = 0;

  // Resume LoRa reception
  LoRa.receive();

  Serial.println(
    "Back to radar. Waiting for next SOS."
  );
}