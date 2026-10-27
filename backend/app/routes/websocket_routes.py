from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from ..core.websocket_manager import manager

router = APIRouter()

@router.websocket("/ws/sos")
async def websocket_sos_endpoint(websocket: WebSocket):
    await manager.connect(websocket)
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        manager.disconnect(websocket)

@router.websocket("/ws/telemetry/{device_id}")
async def websocket_telemetry_endpoint(websocket: WebSocket, device_id: str):
    await manager.connect(websocket)
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        manager.disconnect(websocket)
