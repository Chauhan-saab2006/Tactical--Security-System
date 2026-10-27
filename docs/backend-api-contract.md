# Backend API and WebSocket Contract (Locked for Current Frontend)

This document defines the backend contract that must remain stable so the existing frontend works without any UI code changes.

## Base URLs

- REST: `/api/v1`
- WebSocket: `/ws`

## 1) Telemetry Ingest (Hardware -> Backend)

### `POST /api/v1/telemetry`

Headers:

- `X-Device-Key: <DEVICE_API_KEY>`

Request body:

```json
{
  "device_id": "wrist_001",
  "latitude": 28.6139,
  "longitude": 77.2090,
  "pulse": 92,
  "battery": 84,
  "sos": true,
  "timestamp": "2026-03-24T14:30:00Z"
}
```

Response:

```json
{
  "success": true,
  "event_id": 123,
  "stored_at": "2026-03-24T14:30:00Z"
}
```

Notes:

- Hardware can post every 5 seconds.
- `timestamp` can be server-generated if omitted.

## 2) Active SOS for Frontend Bootstrap

### `GET /api/v1/sos/active`

Response shape must stay compatible with frontend hook parsing:

```json
[
  {
    "id": 123,
    "device_id": "wrist_001",
    "is_active": true,
    "started_at": "2026-03-24T14:25:00Z",
    "pulse_data": [
      {
        "bpm": 92,
        "timestamp": "2026-03-24T14:30:00Z"
      }
    ],
    "location_data": [
      {
        "latitude": 28.6139,
        "longitude": 77.2090,
        "timestamp": "2026-03-24T14:30:00Z"
      }
    ]
  }
]
```

## 3) Resolve SOS

### `POST /api/v1/sos/resolve/{event_id}`

Response:

```json
{
  "success": true,
  "event_id": 123,
  "status": "resolved"
}
```

## 4) Device Status (Panel Polling)

### `GET /api/v1/devices/status`

Response:

```json
[
  {
    "device_id": "wrist_001",
    "is_online": true,
    "last_seen": "2026-03-24T14:30:00Z",
    "last_latitude": 28.6139,
    "last_longitude": 77.2090,
    "last_pulse": 92,
    "sos_active": true
  }
]
```

## 5) Login (Frontend Compatibility)

### `POST /api/v1/login`

Form body:

- `username`
- `password`

Response (minimal compatible):

```json
{
  "access_token": "dummy-or-real-token",
  "token_type": "bearer"
}
```

## 6) WebSocket Stream

### `WS /ws/sos`

Event `sos_alert` (first active SOS or explicit trigger):

```json
{
  "type": "sos_alert",
  "event_id": 123,
  "device_id": "wrist_001",
  "latitude": 28.6139,
  "longitude": 77.2090,
  "pulse": 92,
  "battery": 84,
  "sos": true,
  "timestamp": "2026-03-24T14:30:00Z"
}
```

Event `telemetry_update` (subsequent location updates):

```json
{
  "type": "telemetry_update",
  "event_id": 123,
  "device_id": "wrist_001",
  "latitude": 28.6142,
  "longitude": 77.2093,
  "pulse": 93,
  "battery": 83,
  "sos": true,
  "timestamp": "2026-03-24T14:30:05Z"
}
```

Optional compatibility event:

- `pulse_alert`
- `checkpoint_reached`

## Validation Rules

- Latitude range: `-90..90`
- Longitude range: `-180..180`
- `device_id` non-empty string
- Reject invalid payloads with `422`.
- Reject missing/wrong device key with `401`.

## Performance Target

- End-to-end update latency (ingest -> WS broadcast): under 1 second on local network.
- 5-second hardware updates should be handled continuously without connection drops.
