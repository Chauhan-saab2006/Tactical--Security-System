# Hardware to FastAPI Integration Guide

This project now uses Midway as the bridge from LoRa mesh to FastAPI telemetry.

## Firmware Files to Flash

- Midway gateway: `hardware/midway/final_code_midway.ino`
- Wristband: `hardware/wrist_band/final_code_wristband.ino`
- Relay nodes: `hardware/nodes/final_code_node_a.ino`, `final_code_node_b.ino`, `final_code_node_c.ino`

## Backend Contract Used by Midway

- Endpoint: `POST http://<backend_ip>:8000/api/v1/telemetry`
- Header: `x-device-key: <DEVICE_API_KEY>`
- JSON body:

```json
{
  "device_id": "Wristband",
  "latitude": 29.027916,
  "longitude": 77.059315,
  "pulse": 82,
  "battery": 100,
  "sos": true
}
```

## Current Payload Flow

1. Wristband/Nodes send encrypted LoRa packets (`msgId:origId:hex`).
2. Midway decrypts packet using device-specific key.
3. Midway parses:
   - `RNR` -> SOS active
   - `BPM:x|LAT:y|LON:z|RNR:0/1` -> telemetry update
4. Midway posts normalized JSON to FastAPI.
5. FastAPI stores signal and broadcasts WebSocket updates for map UI.

## Important Configuration

In `final_code_midway.ino`:

- `ssid` and `password`: set your Wi-Fi.
- `DEVICE_API_KEY`: must match backend `DEVICE_API_KEY` in `.env`.
  - current default in firmware is `antigravity_secret_123`.

In backend `.env`:

- `DEVICE_API_KEY=change-me-device-key` (or update both sides to same value).

## Timing

- Wristband periodic send interval is set to 5 seconds.

## Verification Checklist

- Midway serial shows: `Found backend at: <ip>`.
- Midway serial `POST code: 200` for telemetry packets.
- Backend logs show telemetry requests.
- Frontend map updates marker positions in real time.
