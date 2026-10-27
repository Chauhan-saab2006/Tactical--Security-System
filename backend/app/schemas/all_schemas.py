from datetime import datetime
from pydantic import BaseModel, ConfigDict, Field
from typing import Optional

class TelemetryData(BaseModel):
    device_id: str = Field(min_length=1)
    latitude: float = Field(ge=-90, le=90)
    longitude: float = Field(ge=-180, le=180)
    pulse: Optional[int] = None
    battery: Optional[int] = Field(default=None, ge=0, le=100)
    sos: bool = False
    timestamp: Optional[datetime] = None


class EncryptedTelemetry(BaseModel):
    encrypted_data: str
    device_id: Optional[str] = None
    aes_key: Optional[str] = None


class SOSActiveResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: str
    device_id: str
    is_active: bool
    started_at: datetime
    pulse_data: list[dict]
    location_data: list[dict]


class DeviceStatus(BaseModel):
    device_id: str
    is_online: bool
    last_seen: Optional[datetime] = None
    last_latitude: Optional[float] = None
    last_longitude: Optional[float] = None
    last_pulse: Optional[int] = None
    sos_active: bool
