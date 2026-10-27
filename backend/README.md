# FastAPI Backend (Frontend-Compatible)

This backend is built to work with the current frontend without UI changes.

## Run

1. Create/activate virtual env.
2. Install deps:
   - `pip install -r requirements.txt`
3. Copy env:
   - `.env.example` -> `.env`
4. Start server:
   - `uvicorn app.main:app --reload --host 0.0.0.0 --port 8000`

## Main Endpoints

- `POST /api/v1/telemetry` (hardware ingest)
- `POST /api/v1/telemetry/secure` (optional secure ingest)
- `GET /api/v1/sos/active`
- `POST /api/v1/sos/resolve/{event_id}`
- `GET /api/v1/devices/status`
- `POST /api/v1/login`
- `WS /ws/sos`

## Hardware Ingest Example

Headers:
- `X-Device-Key: <DEVICE_API_KEY>`

Body:

```json
{
  "device_id": "wrist_001",
  "latitude": 28.6139,
  "longitude": 77.209,
  "pulse": 92,
  "battery": 84,
  "sos": true
}
```

## Notes

- SQLite is used by default (`sql_app.db`).
- Sending updates every 5 seconds is supported.
- WebSocket events are pushed to `/ws/sos` for real-time map updates.

## Quick Hardware Monitor (Terminal)

To watch incoming signals live in terminal:

- `python tools/live_signal_watch.py`

It prints:
- `device_id`
- `latitude`, `longitude`
- `pulse`
- `sos`
- `received_at`
