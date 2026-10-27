from datetime import datetime, timezone
from ..schemas.all_schemas import TelemetryData
from ..core.websocket_manager import manager
from ..services.pulse_service import pulse_service
from bson import ObjectId

class TelemetryService:
    @staticmethod
    def _to_utc_now():
        return datetime.now(timezone.utc)

    async def process_telemetry(self, db, data: TelemetryData) -> dict:
        timestamp = data.timestamp or self._to_utc_now()

        signal = {
            "device_id": data.device_id,
            "latitude": data.latitude,
            "longitude": data.longitude,
            "pulse": data.pulse,
            "battery": data.battery,
            "sos": data.sos,
            "received_at": timestamp,
        }
        await db.signals.insert_one(signal)

        await pulse_service.validate_pulse(db, data)

        active_event = await db.sos_events.find_one({
            "device_id": data.device_id,
            "is_active": True
        }, sort=[("started_at", -1)])

        event_type = "telemetry_update"
        event_id = str(active_event["_id"]) if active_event else None

        if data.sos:
            if not active_event:
                new_event = {
                    "device_id": data.device_id,
                    "is_active": True,
                    "started_at": timestamp,
                    "last_latitude": data.latitude,
                    "last_longitude": data.longitude,
                    "last_pulse": data.pulse,
                }
                result = await db.sos_events.insert_one(new_event)
                event_id = str(result.inserted_id)
                event_type = "sos_alert"
            else:
                await db.sos_events.update_one(
                    {"_id": active_event["_id"]},
                    {"$set": {
                        "last_latitude": data.latitude,
                        "last_longitude": data.longitude,
                        "last_pulse": data.pulse
                    }}
                )
                event_id = str(active_event["_id"])
        else:
            if active_event:
                await db.sos_events.update_one(
                    {"_id": active_event["_id"]},
                    {"$set": {
                        "is_active": False,
                        "resolved_at": timestamp,
                        "last_latitude": data.latitude,
                        "last_longitude": data.longitude,
                        "last_pulse": data.pulse
                    }}
                )
                event_id = str(active_event["_id"])

        payload = {
            "type": event_type,
            "event_id": event_id,
            "device_id": data.device_id,
            "latitude": data.latitude,
            "longitude": data.longitude,
            "pulse": data.pulse,
            "battery": data.battery,
            "sos": data.sos,
            "timestamp": timestamp.isoformat(),
        }
        await manager.broadcast(payload)
        return payload

telemetry_service = TelemetryService()
