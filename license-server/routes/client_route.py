# license-server/routes/client_route.py
from fastapi import APIRouter, Depends, HTTPException, Query
from pydantic import BaseModel, EmailStr
from sqlalchemy.orm import Session
from sqlalchemy import or_
from datetime import datetime, timedelta
from passlib.hash import bcrypt
from typing import List, Optional

from database import get_db
from models.client import Client
from models.client_credential import ClientCredential
from models.license import License
from utils.license_key import generate_license_key

router = APIRouter(prefix="/api", tags=["clients"])

# ===== Schemas =====
class ProfileIn(BaseModel):
    firstName: str
    lastName: str
    email: EmailStr
    phone: Optional[str] = None
    company: Optional[str] = None
    industry: Optional[str] = None
    country: Optional[str] = None
    message: Optional[str] = None
    estimateUser: Optional[int] = None

class CredIn(BaseModel):
    username: str
    password: str

class TrialIn(BaseModel):
    days: Optional[int] = None

class ClientCreateIn(BaseModel):
    requestType: str  # "trial" | "purchase" | "support"
    search: Optional[str] = None
    source: Optional[str] = None
    sourceId: Optional[str] = None
    profile: ProfileIn
    credentials: CredIn
    trial: TrialIn

class ClientOut(BaseModel):
    id: int
    requestType: str
    firstName: str
    lastName: str
    email: EmailStr
    company: Optional[str] = None
    created_at: Optional[str] = None
    # license fields
    licenseKey: Optional[str] = None
    licenseExpiresAt: Optional[str] = None

def _to_out(c: Client, lic: Optional[License] = None) -> ClientOut:
    return ClientOut(
        id=c.id,
        requestType=c.request_type,
        firstName=c.first_name or "",
        lastName=c.last_name or "",
        email=c.email,
        company=c.company,
        created_at=c.created_at.isoformat() if c.created_at else None,
        licenseKey=lic.license_key if lic else None,
        licenseExpiresAt=lic.expires_at.isoformat() if lic and lic.expires_at else None,
    )

# ===== CREATE =====
@router.post("/clients", response_model=ClientOut)
def create_client(body: ClientCreateIn, db: Session = Depends(get_db)):
    if body.requestType not in {"trial", "purchase", "support"}:
        raise HTTPException(status_code=400, detail=f"Invalid requestType: '{body.requestType}'. Use trial|purchase|support")

    if db.query(Client).filter(Client.email == body.profile.email).first():
        raise HTTPException(status_code=409, detail="Email already exists")
    if db.query(ClientCredential).filter(ClientCredential.username == body.credentials.username).first():
        raise HTTPException(status_code=409, detail="Username already exists")

    now = datetime.utcnow()

    # 1) สร้าง Client
    client = Client(
        request_type=body.requestType,
        source=body.source,
        source_id=body.sourceId,
        first_name=body.profile.firstName,
        last_name=body.profile.lastName,
        email=body.profile.email,
        phone=body.profile.phone,
        company=body.profile.company,
        industry=body.profile.industry,
        country=body.profile.country,
        message=body.profile.message,
        estimate_user=body.profile.estimateUser,
        trial_days=body.trial.days,
        created_at=now,
    )
    db.add(client)
    db.flush()  # ให้ได้ client.id

    # 2) สร้าง Credential
    cred = ClientCredential(
        client_id=client.id,
        username=body.credentials.username,
        password_hash=bcrypt.hash(body.credentials.password),
        created_at=now,
    )
    db.add(cred)

    # 3) สร้าง License Key ทันที
    # - ถ้า trial: ใช้ days ที่ส่งมา (default 15 วัน)
    # - purchase/support: ออกคีย์แบบ perpetual (ไม่กำหนดวันหมดอายุ) หรือจะปรับเป็น subscription ได้ภายหลัง
    license_key = generate_license_key()
    term = "trial" if body.requestType == "trial" else "perpetual"
    product_sku = "SMART_AUDIT_TRIAL" if term == "trial" else None

    duration_days = None
    expires_at = None
    if term == "trial":
        duration_days = body.trial.days or 15
        expires_at = now + timedelta(days=duration_days)

    lic = License(
        client_id=client.id,
        license_key=license_key,
        term=term,
        product_sku=product_sku,
        duration_days=duration_days,
        max_activations=1,
        activations_used=0,
        status="active",
        issued_at=now,
        expires_at=expires_at,
        created_at=now,
    )
    db.add(lic)

    db.commit()
    db.refresh(client)
    db.refresh(lic)

    return _to_out(client, lic)

# ===== LIST =====
class ClientListItem(BaseModel):
    id: int
    firstName: str
    lastName: str
    email: EmailStr
    company: Optional[str] = None
    requestType: str
    created_at: Optional[str] = None

@router.get("/clients", response_model=List[ClientListItem])
def list_clients(
    db: Session = Depends(get_db),
    q: Optional[str] = Query(None, description="keyword: name/email/company"),
    type: Optional[str] = Query(None, pattern="^(trial|purchase|support)$"),
    limit: int = Query(200, ge=1, le=500),
    offset: int = Query(0, ge=0),
):
    qry = db.query(Client)
    if q:
        kw = f"%{q.strip()}%"
        qry = qry.filter(or_(
            Client.first_name.ilike(kw),
            Client.last_name.ilike(kw),
            Client.email.ilike(kw),
            Client.company.ilike(kw),
        ))
    if type:
        qry = qry.filter(Client.request_type == type)

    rows = qry.order_by(Client.id.desc()).offset(offset).limit(limit).all()
    return [
        ClientListItem(
            id=c.id,
            firstName=c.first_name or "",
            lastName=c.last_name or "",
            email=c.email,
            company=c.company,
            requestType=c.request_type,
            created_at=c.created_at.isoformat() if c.created_at else None,
        )
        for c in rows
    ]

# ===== GET BY ID =====
@router.get("/clients/{client_id}", response_model=ClientOut)
def get_client(client_id: int, db: Session = Depends(get_db)):
    c = db.query(Client).filter(Client.id == client_id).first()
    if not c:
        raise HTTPException(status_code=404, detail="Client not found")

    lic = (
        db.query(License)
        .filter(License.client_id == client_id)
        .order_by(License.id.desc())
        .first()
    )
    return _to_out(c, lic)
