import json
from datetime import datetime, timezone
from typing import List
from fastapi import APIRouter, Depends, Header, HTTPException
from ..database import get_db
from ..schemas.all_schemas import TelemetryData, DeviceStatus, EncryptedTelemetry
from ..services.telemetry_service import telemetry_service
from ..config import settings
from ..core.security import decrypt_aes_256_cbc_json

router = APIRouter()

def verify_device_key(x_device_key: str = Header(..., alias="X-Device-Key")):
    if x_device_key != settings.DEVICE_API_KEY:
        raise HTTPException(status_code=401, detail="Invalid Device API Key")

@router.get("/devices/status", response_model=List[DeviceStatus])
async def get_devices_status(db = Depends(get_db)):
    # Aggregate to get the latest signal per device
    pipeline = [
        {"$sort": {"received_at": -1}},
        {"$group": {
            "_id": "$device_id",
            "last_signal": {"$first": "$$ROOT"}
        }}
    ]
    cursor = db.signals.aggregate(pipeline)
    
    statuses = []
    now = datetime.now(timezone.utc)
    
    async for doc in cursor:
        device_id = doc["_id"]
        signal = doc["last_signal"]
        
        # Check if there's an active SOS event for this device
        active_event = await db.sos_events.find_one({
            "device_id": device_id,
            "is_active": True
        })
        
        received_at = signal["received_at"]
        if received_at.tzinfo is None:
            received_at = received_at.replace(tzinfo=timezone.utc)
            
        delta = now - received_at
        
        statuses.append(
            DeviceStatus(
                device_id=device_id,
                is_online=delta.total_seconds() <= 30,
                last_seen=received_at,
                last_latitude=signal.get("latitude"),
                last_longitude=signal.get("longitude"),
                last_pulse=signal.get("pulse"),
                sos_active=bool(active_event),
            )
        )
    return statuses

@router.post("/telemetry/secure", dependencies=[Depends(verify_device_key)])
async def post_secure_telemetry(payload: EncryptedTelemetry, db = Depends(get_db)):
    decrypted_json = decrypt_aes_256_cbc_json(payload.encrypted_data, aes_key=payload.aes_key)
    if decrypted_json is None:
        try:
            decrypted_json = json.loads(payload.encrypted_data)
        except Exception as exc:
            raise HTTPException(status_code=400, detail="Could not decrypt/parse secure payload") from exc

    if payload.device_id and not decrypted_json.get("device_id"):
        decrypted_json["device_id"] = payload.device_id
    data = TelemetryData(**decrypted_json)
    ws_payload = await telemetry_service.process_telemetry(db, data)
    return {"success": True, "event_id": ws_payload.get("event_id"), "stored_at": ws_payload["timestamp"]}

@router.post("/telemetry", dependencies=[Depends(verify_device_key)])
async def post_telemetry(data: TelemetryData, db = Depends(get_db)):
    ws_payload = await telemetry_service.process_telemetry(db, data)
    return {"success": True, "event_id": ws_payload.get("event_id"), "stored_at": ws_payload["timestamp"]}
