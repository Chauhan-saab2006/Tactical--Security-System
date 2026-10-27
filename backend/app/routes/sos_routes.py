from fastapi import APIRouter, Depends, HTTPException
from ..database import get_db
from datetime import datetime, timezone
from ..schemas.all_schemas import SOSActiveResponse
from typing import List
from bson import ObjectId

router = APIRouter()

@router.get("/sos/active", response_model=List[SOSActiveResponse])
async def get_active_sos(db = Depends(get_db)):
    cursor = db.sos_events.find({"is_active": True}).sort("started_at", -1)
    
    response = []
    async for event in cursor:
        started_at = event["started_at"]
        if started_at.tzinfo is None:
            started_at = started_at.replace(tzinfo=timezone.utc)
            
        response.append(
            SOSActiveResponse(
                id=str(event["_id"]),
                device_id=event["device_id"],
                is_active=event["is_active"],
                started_at=started_at,
                pulse_data=[{"bpm": event.get("last_pulse"), "timestamp": started_at.isoformat()}],
                location_data=[
                    {
                        "latitude": event.get("last_latitude"),
                        "longitude": event.get("last_longitude"),
                        "timestamp": started_at.isoformat(),
                    }
                ],
            )
        )
    return response

@router.post("/sos/resolve/{event_id}")
async def resolve_sos(event_id: str, db = Depends(get_db)):
    try:
        obj_id = ObjectId(event_id)
    except Exception:
        raise HTTPException(status_code=400, detail="Invalid event ID format")

    result = await db.sos_events.update_one(
        {"_id": obj_id},
        {"$set": {
            "is_active": False,
            "resolved_at": datetime.now(timezone.utc)
        }}
    )
    
    if result.matched_count == 0:
        raise HTTPException(status_code=404, detail="SOS Event not found")

    return {"success": True, "event_id": event_id, "status": "resolved"}
