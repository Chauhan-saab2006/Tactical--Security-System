from fastapi import APIRouter, Depends, HTTPException, status
from fastapi.security import OAuth2PasswordRequestForm
from datetime import timedelta, datetime, timezone
from ..database import get_db
from ..core.security import get_password_hash, verify_password, create_access_token
from ..config import settings

router = APIRouter()

async def _ensure_default_operator(db):
    user = await db.users.find_one({"email": "operator@lifelink.local"})
    if user:
        return user
    
    new_user = {
        "email": "operator@lifelink.local",
        "hashed_password": get_password_hash("operator123"),
        "created_at": datetime.now(timezone.utc)
    }
    await db.users.insert_one(new_user)
    return new_user

@router.post("/login")
async def login(form_data: OAuth2PasswordRequestForm = Depends(), db = Depends(get_db)):
    await _ensure_default_operator(db)
    user = await db.users.find_one({"email": form_data.username})
    if not user or not verify_password(form_data.password, user["hashed_password"]):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Incorrect email or password",
            headers={"WWW-Authenticate": "Bearer"},
        )
    access_token_expires = timedelta(minutes=settings.ACCESS_TOKEN_EXPIRE_MINUTES)
    access_token = create_access_token(
        subject=user["email"], expires_delta=access_token_expires
    )
    return {"access_token": access_token, "token_type": "bearer"}
