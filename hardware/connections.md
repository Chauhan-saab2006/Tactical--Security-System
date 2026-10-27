# Antigravity SOS Hardware Connections

This document details the hardware connections for all units in the Antigravity SOS system.

## 1. Common LoRa Connection (All Devices)
*Shared SPI bus used in Nodes, Midway Panel, and Wristbands.*

**Module:** LoRa SX1278 / SX1276 (433 MHz)

| LoRa Pin | ESP32 Pin |
| :------- | :-------- |
| VSS      | 3.3V      |
| GND      | GND       |
| NSS      | GPIO 5    |
| SCK      | GPIO 18   |
| MISO     | GPIO 19   |
| MOSI     | GPIO 23   |
| RST      | GPIO 14   |
| DIO0     | GPIO 26   |

---

## 2. OLED Connection (All Devices)
*Shared I2C bus used in Nodes, Midway Panel, and Wristbands.*

**Module:** 0.96” SSD1306 (I2C)

| OLED Pin | ESP32 Pin |
| :------- | :-------- |
| VSS      | 3.3V      |
| GND      | GND       |
| SDA      | GPIO 4    |
| SCL      | GPIO 15   |

---

## 3. Node Unit (Tower)
*Components: ESP32, LoRa, OLED, SOS Button, RFID.*

### SOS Button
| SOS Pin | ESP32 Pin |
| :------ | :-------- |
| GND     | GND       |
| Signal  | GPIO 13   |

### RFID Module (Red I2C Version)
| RFID Pin | ESP32 Pin |
| :------- | :-------- |
| VCC      | 3.3V      |
| GND      | GND       |
| SDA      | GPIO 16   |
| SCL      | GPIO 17   |

---

## 4. Midway Panel (Control Panel)
*Components: ESP32, LoRa, OLED, Traffic Light Module.*

### Traffic Light Module
| Traffic Pin | ESP32 Pin | Usage           |
| :---------- | :-------- | :-------------- |
| GND         | GND       | Common Ground   |
| Red         | GPIO 27   | SOS alert       |
| Yellow      | GPIO 25   | Warning         |
| Green       | GPIO 33   | Safe            |

---

## 5. Wristband Unit
*Components: ESP32, LoRa, OLED, GPS.*

### GPS Module (UART Communication)
| GPS Pin | ESP32 Pin |
| :------ | :-------- |
| GND     | GND       |
| VCC     | 3.3V      |
| TX      | GPIO 16   |
| RX      | GPIO 17   |

---

## Summary Table

| Device       | Core | Comm (SPI) | Display (I2C) | Sensors/IO |
| :----------- | :--- | :--------- | :------------ | :--------- |
| **Node**     | ESP32| LoRa (5, 18, 19, 23, 14, 26) | OLED (4, 15) | Button (13), RFID (16, 17 I2C) |
| **Midway**   | ESP32| LoRa (5, 18, 19, 23, 14, 26) | OLED (4, 15) | Traffic (27, 25, 33) |
| **Wristband**| ESP32| LoRa (5, 18, 19, 23, 14, 26) | OLED (4, 15) | GPS (16, 17 UART) |
