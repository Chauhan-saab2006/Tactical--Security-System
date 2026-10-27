# Backend Implementation Checklist (FastAPI SOS Rebuild)

Use this as the build lock checklist while implementing.

## Scope Lock

- [ ] Frontend files remain untouched.
- [ ] Single backend runtime: FastAPI in `backend/`.
- [ ] Keep REST/WS contracts defined in `docs/backend-api-contract.md`.

## Project Cleanup

- [ ] Remove or archive old backend code paths not part of new design.
- [ ] Remove stale generated/debug artifacts from backend source tree.
- [ ] Add/update `.gitignore` for `.pyc`, logs, local db files as needed.

## Foundation

- [ ] `requirements.txt` with FastAPI stack:
  - `fastapi`, `uvicorn`, `sqlalchemy`, `pydantic`, `python-dotenv`, `passlib`, `python-multipart`
- [ ] `app/main.py` with app init, CORS, router mounting.
- [ ] `app/config.py` for env settings.
- [ ] `app/database.py` for engine/session/base.
- [ ] Startup table creation (or migration bootstrap).

## Data Layer

- [ ] Implement `signals` table model.
- [ ] Implement `sos_events` table model.
- [ ] Implement optional minimal `users` table model.
- [ ] Add indexes for `device_id`, `received_at`, active SOS lookup.

## Schemas

- [ ] Telemetry ingest schema.
- [ ] SOS active response schema with `pulse_data` and `location_data`.
- [ ] Device status schema.
- [ ] Login response schema.

## Routes

- [ ] `POST /api/v1/telemetry`
- [ ] `GET /api/v1/sos/active`
- [ ] `POST /api/v1/sos/resolve/{event_id}`
- [ ] `GET /api/v1/devices/status`
- [ ] `POST /api/v1/login` (compatibility)
- [ ] `GET /` health/info endpoint

## Real-Time

- [ ] `WebSocketManager` with connection tracking.
- [ ] `WS /ws/sos` endpoint.
- [ ] Broadcast `sos_alert` and `telemetry_update` events.
- [ ] Graceful disconnect handling.

## Business Logic

- [ ] On each telemetry packet: validate -> store signal.
- [ ] If `sos=true` and no active event for device: create event + broadcast `sos_alert`.
- [ ] If active SOS exists: update event location/pulse + broadcast `telemetry_update`.
- [ ] On resolve endpoint: close event and persist status.

## Security and Config

- [ ] Validate `X-Device-Key` on telemetry ingest.
- [ ] Keep secrets in `.env` only.
- [ ] Add `.env.example` with safe placeholders.
- [ ] Restrict CORS based on deployment env.

## QA and Verification

- [ ] Manual ingest test every 5 sec using curl/script.
- [ ] Verify map marker updates live in existing frontend.
- [ ] Verify active SOS appears on refresh.
- [ ] Verify resolve flow removes/updates active incident.
- [ ] Verify DB rows created correctly.

## Operational Readiness

- [ ] Backend `README.md` with run commands.
- [ ] Add `curl` examples for telemetry and SOS resolve.
- [ ] Define expected hardware payload examples.
- [ ] Add simple restart-safe behavior notes.

## Done Criteria

- [ ] Hardware -> FastAPI -> DB -> WebSocket -> Frontend map works end-to-end.
- [ ] No frontend code modifications required.
- [ ] Backend structure is clean, documented, and stable for future extension.
