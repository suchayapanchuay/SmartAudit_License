# utils/jwt.py
from datetime import datetime, timedelta, timezone
from typing import Optional, Dict, Any
import jwt  # PyJWT
from license_server.utils.settings import settings

ALGORITHM = "HS256"
ACCESS_TOKEN_EXPIRE_HOURS = 24

def create_access_token(data: Dict[str, Any], expires_delta: Optional[timedelta] = None) -> str:
    to_encode = data.copy()
    expire = datetime.now(timezone.utc) + (expires_delta or timedelta(hours=ACCESS_TOKEN_EXPIRE_HOURS))
    to_encode.update({"exp": expire})
    secret = getattr(settings, "JWT_SECRET", None) or getattr(settings, "SECRET_KEY", None) or "CHANGE_ME"
    return jwt.encode(to_encode, secret, algorithm=ALGORITHM)

def decode_token(token: str) -> Dict[str, Any]:
    secret = getattr(settings, "JWT_SECRET", None) or getattr(settings, "SECRET_KEY", None) or "CHANGE_ME"
    return jwt.decode(token, secret, algorithms=[ALGORITHM])
