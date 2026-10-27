# FastAPI Backend Rebuild Plan (Frontend-Compatible)

## Goal

Build a clean, stable FastAPI backend for SOS telemetry that:

- keeps the current frontend unchanged,
- receives SOS/location signals from hardware,
- stores signals in a small database,
- streams live location updates to map UI in real time.

## Non-Negotiables

- Do not modify frontend code.
- Keep REST and WebSocket URLs compatible with existing frontend:
  - REST base: `/api/v1`
  - WS base: `/ws`
- Support location updates at 5-second intervals from hardware.
- Use Python + FastAPI + SQLite (simple and sufficient for now).

## Frontend Contract (Already in Use)

The current UI expects these endpoints/events:

- `GET /api/v1/sos/active`
- `POST /api/v1/sos/resolve/{event_id}`
- `GET /api/v1/devices/status`
- `POST /api/v1/login` (can stay minimal/mock initially)
- `WS /ws/sos` (primary real-time stream)

Expected WS event types consumed by UI:

- `sos_alert`
- `telemetry_update`
- `pulse_alert` (optional but safe to keep)
- `checkpoint_reached` (optional)

## Recommended Clean Backend Structure

Use one backend only (`backend/`) and remove runtime dependency on old Node backend.

```text
backend/
  app/
    main.py
    config.py
    database.py
    models.py
    schemas.py
    deps.py
    core/
      websocket_manager.py
    routes/
      auth.py
      telemetry.py
      sos.py
      websocket.py
    services/
      telemetry_service.py
      sos_service.py
  requirements.txt
  .env.example
  README.md
```

## Data Model (Minimal and Stable)

Keep schema simple:

1. `signals` table (raw incoming data)
   - `id` (pk)
   - `device_id` (indexed)
   - `latitude` (float)
   - `longitude` (float)
   - `sos` (bool)
   - `pulse` (nullable int)
   - `battery` (nullable int)
   - `received_at` (datetime, indexed)

2. `sos_events` table (active/closed incidents)
   - `id` (pk)
   - `device_id` (indexed)
   - `is_active` (bool, indexed)
   - `started_at` (datetime)
   - `resolved_at` (nullable datetime)
   - `last_latitude` (float)
   - `last_longitude` (float)
   - `last_pulse` (nullable int)

3. `users` table (optional minimal auth compatibility)
   - `id`, `email`, `hashed_password`, `created_at`

## Runtime Flow

1. Hardware sends telemetry every 5 seconds.
2. Backend validates and stores signal in `signals`.
3. If `sos=true`, create/update active `sos_events` row for that device.
4. Broadcast over `/ws/sos`:
   - first signal with SOS -> `sos_alert`
   - subsequent updates -> `telemetry_update`
5. Frontend map updates marker position in real time.

## API and WebSocket Compatibility Rules

To avoid frontend changes:

- Keep `device_id`, `latitude`, `longitude`, `pulse`, `timestamp`, `event_id` fields in WS payloads.
- `GET /sos/active` should return each active event with nested recent location/pulse data compatible with existing hook parsing.
- `resolve` endpoint must return success JSON and mark event inactive.

## Implementation Phases

### Phase 1 - Clean Foundation

- Remove dead/legacy backend code paths.
- Initialize FastAPI app, config, DB session, models, startup checks.
- Add health endpoint.

### Phase 2 - Ingestion + Storage

- Build telemetry ingest endpoint with API key auth.
- Validate payload with Pydantic.
- Persist each signal row.

### Phase 3 - SOS Logic + Real-Time

- Add SOS event lifecycle logic.
- Add WebSocket manager and `/ws/sos`.
- Broadcast `sos_alert` and `telemetry_update`.

### Phase 4 - Frontend Compatibility Endpoints

- Implement `/sos/active`, `/sos/resolve/{event_id}`, `/devices/status`, `/login`.
- Verify response shape against current frontend code.

### Phase 5 - Stability

- Add structured logging and clear error responses.
- Add basic tests for telemetry ingest + websocket event generation.
- Add run instructions and integration checklist.

## Acceptance Criteria

- Frontend map receives live marker updates without UI edits.
- New SOS appears as alert within 1 update cycle (<=5 sec).
- Coordinates persist in DB for audit/history.
- Active SOS list refreshes correctly on page reload.
- Backend restarts cleanly and serves same contracts.

## Risk Controls

- Keep one canonical protocol for hardware payload fields.
- Avoid hardcoded secrets; use `.env`.
- Do not keep generated logs or `.pyc` in git.
- Defer advanced features (multi-tenant auth, analytics) until stable core works.
