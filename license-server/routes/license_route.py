# license-server/routes/license_route.py
from fastapi import APIRouter, Depends, HTTPException, Query
from pydantic import BaseModel
from datetime import datetime, timedelta
from sqlalchemy.orm import Session
from typing import List, Optional
import secrets
import string

from database import get_db
from models.license import License
from utils.logging import log_action

router = APIRouter(prefix="/api", tags=["licenses"])

ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"  # ตัด 1, I, O, 0
def generate_license_key(groups: int = 4, chars_per_group: int = 5) -> str:
    def chunk():
        return "".join(secrets.choice(ALPHABET) for _ in range(chars_per_group))
    return "-".join(chunk() for _ in range(groups))

# ---------- Schemas ----------
class LicenseCreateRequest(BaseModel):
    client_id: int
    term: str = "trial"                # trial | subscription | perpetual
    product_sku: Optional[str] = None
    duration_days: Optional[int] = 30  # ใช้กับ trial/subscription
    max_activations: int = 1

class LicenseOut(BaseModel):
    id: int
    client_id: int
    license_key: str
    term: str
    product_sku: Optional[str] = None
    duration_days: Optional[int] = None
    max_activations: int
    activations_used: int
    status: str
    issued_at: Optional[datetime] = None
    expires_at: Optional[datetime] = None
    created_at: Optional[datetime] = None

    class Config:
        orm_mode = True

# ---------- Create ----------
@router.post("/licenses", response_model=LicenseOut)
def create_license(req: LicenseCreateRequest, db: Session = Depends(get_db)):
    # unique key
    for _ in range(5):
        license_key = generate_license_key()
        if not db.query(License).filter(License.license_key == license_key).first():
            break
    else:
        raise HTTPException(status_code=500, detail="Failed to generate unique license key")

    now = datetime.utcnow()
    expires = None
    duration = None

    term = req.term or "trial"
    if term == "trial":
        duration = req.duration_days or 15
        expires = now + timedelta(days=duration)
    elif term == "subscription":
        duration = req.duration_days or 30
        expires = now + timedelta(days=duration)
    else:
        # perpetual
        duration = None
        expires = None

    lic = License(
        client_id=req.client_id,
        license_key=license_key,
        term=term,
        product_sku=req.product_sku,
        duration_days=duration,
        max_activations=req.max_activations,
        activations_used=0,
        status="active",
        issued_at=now,
        expires_at=expires,
        created_at=now,
    )
    db.add(lic)
    db.commit()
    db.refresh(lic)

    log_action(db, f"create license {lic.license_key} for client {lic.client_id}")
    return lic

# ---------- List (optional filter by clientId) ----------
@router.get("/licenses", response_model=List[LicenseOut])
def list_licenses(
    db: Session = Depends(get_db),
    clientId: Optional[int] = Query(None),
):
    q = db.query(License)
    if clientId:
        q = q.filter(License.client_id == clientId)
    return q.order_by(License.id.desc()).all()

# ---------- Get by id ----------
@router.get("/licenses/{license_id}", response_model=LicenseOut)
def get_license(license_id: int, db: Session = Depends(get_db)):
    lic = db.query(License).filter(License.id == license_id).first()
    if not lic:
        raise HTTPException(status_code=404, detail="License not found")
    return lic

# ---------- Delete ----------
@router.delete("/licenses/{license_id}")
def delete_license(license_id: int, db: Session = Depends(get_db)):
    lic = db.query(License).filter(License.id == license_id).first()
    if not lic:
        raise HTTPException(status_code=404, detail="License not found")

    db.delete(lic)
    db.commit()
    log_action(db, f"delete license {license_id}")

    return {"ok": True}
