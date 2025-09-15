# routes/trial_requests.py
from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session
from pydantic import BaseModel, EmailStr, Field
from datetime import datetime
from typing import Optional, List

from database import get_db
from models.trial_request import TrialRequest as TrialRequestModel
from utils.events import publish  # ต้องมีฟังก์ชัน publish(event_name: str, data: dict)

router = APIRouter()

# ---------- Schemas ----------
class TrialRequestIn(BaseModel):
    firstName: str = Field(min_length=1)
    lastName:  str = Field(min_length=1)
    email:     EmailStr
    phone:     str
    company:   str
    industry:  str
    country:   str
    jobTitle:  str
    message:   Optional[str] = None
    utm:       Optional[dict] = None

class TrialRequestOut(BaseModel):
    id:        int
    firstName: str
    lastName:  str
    email:     EmailStr
    phone:     str
    company:   str
    industry:  str
    country:   str
    jobTitle:  str
    message:   Optional[str] = None
    utm:       Optional[dict] = None
    created_at: Optional[datetime] = None
    deleted_at: Optional[datetime] = None

    class Config:
        from_attributes = True  # pydantic v2; ถ้า v1 -> orm_mode = True

# ---------- List ----------
@router.get("/trial-requests", response_model=List[TrialRequestOut])
def list_trial_requests(db: Session = Depends(get_db)):
    rows = (
        db.query(TrialRequestModel)
        .filter(TrialRequestModel.deleted_at.is_(None))
        .order_by(TrialRequestModel.created_at.desc())
        .all()
    )
    return rows

# ---------- Create ----------
@router.post("/trial-requests", response_model=TrialRequestOut)
async def create_trial_request(req: TrialRequestIn, db: Session = Depends(get_db)):
    obj = TrialRequestModel(**req.model_dump())
    db.add(obj)
    db.commit()
    db.refresh(obj)

    # ส่ง event จาก "object ที่บันทึกแล้ว" เพื่อให้มี id/created_at ครบ
    # แปลงเป็น dict ที่ serializable (pydantic ช่วย)
    out = TrialRequestOut.model_validate(obj).model_dump()
    await publish("trial_request", out)

    return obj

# ---------- Soft delete ----------
@router.delete("/trial-requests/{request_id}")
def delete_trial_request(request_id: int, db: Session = Depends(get_db)):
    trial = (
        db.query(TrialRequestModel)
        .filter(
            TrialRequestModel.id == request_id,
            TrialRequestModel.deleted_at.is_(None),
        )
        .first()
    )
    if not trial:
        raise HTTPException(status_code=404, detail="Trial request not found")

    trial.deleted_at = datetime.utcnow()
    db.commit()
    return {"ok": True, "message": "deleted"}
