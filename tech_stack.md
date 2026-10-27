# Technology Stack - LifeLine SOS System

This document outlines the technologies used across the different layers of the Antigravity SOS (LifeLine) project.

## 🎨 Frontend (Dashboard & Monitoring)
- **Framework:** [React 19](https://react.dev/)
- **Build Tool:** [Vite](https://vitejs.dev/)
- **State Management:** React Hooks (`useState`, `useEffect`)
- **Real-time Communication:** [WebSockets](https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API)
- **Maps:** 
  - [React-Leaflet](https://react-leaflet.js.org/) (OpenStreetMap)
  - [@vis.gl/react-google-maps](https://visgl.github.io/react-google-maps/) (Google Maps integration)
- **Icons:** [Lucide React](https://lucide.dev/)
- **Styling:** Vanilla CSS (Modern CSS variables and animations)

## ⚙️ Backend (API & Processing)
- **Framework:** [Node.js / Express](https://expressjs.com/) (Extremely fast JSON/IoT event networking)
- **Database:** [MongoDB](https://www.mongodb.com/) (NoSQL document storage, highly flexible schemas)
- **ORM / Schema Definition:** [Mongoose](https://mongoosejs.com/) (Provides a structured table definition inside Python/js cleanly)
- **Hardware Integration Layer:** Python `uvicorn` instance safely legacy-available but effectively replaced by Express routing structure.

## 🔌 Hardware (Nodes & Midway Gateway)
- **Microcontrollers:** ESP32 (WROOM/DevKit)
- **Programming Environment:** Arduino IDE / C++
- **Communication:**
  - **Off-Grid:** LoRa (Long Range 433MHz) using the `LoRa.h` library.
  - **Internet-Facing:** Wi-Fi (`WiFi.h`)
- **Sensors & Peripherals:**
  - **Displays:** OLED SSD1306 (128x64) via I2C (`Adafruit_SSD1306`).
  - **Scanning:** RFID MFRC522 (`MFRC522.h`).
  - **Vitals:** Digital Heart Rate Pulse Sensor.
- **Protocol:** HTTP POST (JSON payloads) for telemetry ingestion.

## 🔄 Integrations
