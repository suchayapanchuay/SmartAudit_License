# utils/deps.py
from fastapi import Depends, HTTPException
from fastapi.security import HTTPBearer, HTTPAuthorizationCredentials
from sqlalchemy.orm import Session
from database import get_db
from utils.jwt import decode_token
from models.client import Client

security = HTTPBearer(auto_error=True)

def get_current_client(
    cred: HTTPAuthorizationCredentials = Depends(security),
    db: Session = Depends(get_db),
) -> Client:
    if not cred or cred.scheme.lower() != "bearer":
        raise HTTPException(status_code=401, detail="Not authenticated")
    try:
        payload = decode_token(cred.credentials)
        client_id = int(payload.get("sub"))
    except Exception:
        raise HTTPException(status_code=401, detail="Invalid or expired token")

    c = db.query(Client).filter(Client.id == client_id).first()
    if not c:
        raise HTTPException(status_code=401, detail="Client not found")
    return c
