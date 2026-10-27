from ..schemas.all_schemas import TelemetryData
from ..core.websocket_manager import manager

class PulseService:
    @staticmethod
    async def validate_pulse(db, data: TelemetryData):
        if data.pulse is None:
            return False

        is_abnormal = False
        alert_type = "normal"
        reason = ""

        last_reading = await db.signals.find_one(
            {"device_id": data.device_id, "pulse": {"$ne": None}},
            sort=[("received_at", -1)]
        )

        if last_reading and last_reading.get("pulse") is not None:
            diff = abs(data.pulse - last_reading["pulse"])
            if diff > 20:
                is_abnormal = True
                alert_type = "critical"
                reason = f"Critical Pulse Fluctuation (+{diff} BPM)"

        if not is_abnormal:
            if data.pulse > 100:
                is_abnormal = True
                alert_type = "high_pulse"
                reason = "High Pulse (Tachycardia)"
            elif data.pulse < 60:
                is_abnormal = True
                alert_type = "low_pulse"
                reason = "Low Pulse (Bradycardia)"

        if is_abnormal:
            # Broadcast abnormal pulse event
            await manager.broadcast({
                "type": "pulse_alert",
                "device_id": data.device_id,
                "pulse": data.pulse,
                "alert_level": alert_type,
                "reason": reason,
                "timestamp": (data.timestamp.isoformat() if data.timestamp else None)
            })
            
        return is_abnormal

pulse_service = PulseService()
