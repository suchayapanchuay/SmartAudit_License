# routes/auth_route.py
from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel
from sqlalchemy.orm import Session
from passlib.hash import bcrypt

from database import get_db
from models.client_credential import ClientCredential
from models.license import License
from utils.jwt import create_access_token
from utils.deps import get_current_client

router = APIRouter(prefix="/api", tags=["auth"])

# ---------- Login ----------
class LoginIn(BaseModel):
    username: str
    password: str

class LoginOut(BaseModel):
    access_token: str
    token_type: str = "bearer"

@router.post("/login", response_model=LoginOut)
def login(data: LoginIn, db: Session = Depends(get_db)):
    cred = db.query(ClientCredential).filter(ClientCredential.username == data.username).first()
    if not cred or not bcrypt.verify(data.password, cred.password_hash):
        raise HTTPException(status_code=400, detail="Invalid credentials")

    token = create_access_token({"sub": str(cred.client_id), "username": cred.username})
    return LoginOut(access_token=token)

# ---------- Me / Profile ----------
class MeOut(BaseModel):
    id: int
    firstName: str
    lastName: str
    email: str
    company: str | None = None
    phone: str | None = None
    country: str | None = None
    requestType: str

@router.get("/me", response_model=MeOut)
def me(current = Depends(get_current_client)):
    return MeOut(
        id=current.id,
        firstName=current.first_name or "",
        lastName=current.last_name or "",
        email=current.email,
        company=current.company,
        phone=current.phone,
        country=current.country,
        requestType=current.request_type,
    )

# ---------- Licenses of current user ----------
@router.get("/me/licenses")
def my_licenses(current = Depends(get_current_client), db: Session = Depends(get_db)):
    rows = (
        db.query(License)
        .filter(License.client_id == current.id)
        .order_by(License.id.desc())
        .all()
    )
    return [
        {
            "id": lic.id,
            "licenseKey": lic.license_key,
            "type": lic.term,
            "productSku": lic.product_sku or "-",
            "issuedAt": lic.issued_at.isoformat() if lic.issued_at else None,
            "expiresAt": lic.expires_at.isoformat() if lic.expires_at else None,
            "status": lic.status,
        }
        for lic in rows
    ]
