from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel
from datetime import datetime, timedelta
from sqlalchemy.orm import Session
import random, string
from typing import List, Optional

from database import get_db
from models.license import License

router = APIRouter()

def generate_license_key():
    return '-'.join(''.join(random.choices(string.ascii_uppercase + string.digits, k=4)) for _ in range(4))

class LicenseCreateRequest(BaseModel):
    product_name: str
    license_type_id: Optional[int] = None
    user_id: Optional[int] = None
    duration_days: int = 30
    max_activations: int = 3

class LicenseCreateResponse(BaseModel):
    license_key: str
    product_name: str
    expire_date: datetime
    status: str

class LicenseOut(BaseModel):
    id: int
    license_key: str
    product_name: str
    user_id: Optional[int]
    license_type_id: Optional[int]
    issued_date: datetime
    expire_date: datetime
    max_activations: int
    activations: int
    status: str
    created_at: datetime
    updated_at: datetime

    class Config:
        orm_mode = True

@router.post("/license", response_model=LicenseCreateResponse)
def create_license(req: LicenseCreateRequest, db: Session = Depends(get_db)):
    if not req.product_name:
        raise HTTPException(status_code=400, detail="product_name is required")

    # สร้าง license key ที่ไม่ซ้ำ
    for _ in range(5):
        license_key = generate_license_key()
        existing = db.query(License).filter(License.license_key == license_key).first()
        if not existing:
            break
    else:
        raise HTTPException(status_code=500, detail="Failed to generate unique license key")

    now = datetime.utcnow()
    expire = now + timedelta(days=req.duration_days)

    license = License(
        license_key=license_key,
        product_name=req.product_name,
        user_id=req.user_id,
        license_type_id=req.license_type_id,
        issued_date=now,
        expire_date=expire,
        max_activations=req.max_activations,
        activations=0,
        status="active",
        created_at=now,
        updated_at=now,
    )

    db.add(license)
    db.commit()
    db.refresh(license)

    return LicenseCreateResponse(
        license_key=license.license_key,
        product_name=license.product_name,
        expire_date=license.expire_date,
        status=license.status
    )

@router.get("/licenses", response_model=List[LicenseOut])
def get_licenses(db: Session = Depends(get_db)):
    licenses = db.query(License).all()
    return licenses

@router.delete("/license/{license_id}")
def delete_license(license_id: int, db: Session = Depends(get_db)):
    license = db.query(License).filter(License.id == license_id).first()
    if not license:
        raise HTTPException(status_code=404, detail="License not found")
    db.delete(license)
    db.commit()
    return {"message": "License deleted successfully"}


