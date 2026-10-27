from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from .config import settings
from .routes import telemetry_routes, websocket_routes, sos_routes, auth_routes

app = FastAPI(
    title=settings.PROJECT_NAME,
    openapi_url=f"{settings.API_V1_STR}/openapi.json"
)

cors_origins = [origin.strip() for origin in settings.CORS_ORIGINS.split(",")] if settings.CORS_ORIGINS else ["*"]
app.add_middleware(
    CORSMiddleware,
    allow_origins=cors_origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(auth_routes.router, prefix=settings.API_V1_STR, tags=["auth"])
app.include_router(telemetry_routes.router, prefix=settings.API_V1_STR, tags=["telemetry"])
app.include_router(sos_routes.router, prefix=settings.API_V1_STR, tags=["sos"])
app.include_router(websocket_routes.router, tags=["websocket"])

@app.get("/")
async def root():
    return {"message": "Life Link FastAPI backend is running", "api_prefix": settings.API_V1_STR}
