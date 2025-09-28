# routes/trial_requests.py
from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session
from pydantic import BaseModel, EmailStr, Field
from datetime import datetime, timezone
from typing import Optional, List

from database import get_db
from models.trial_request import TrialRequest as TrialRequestModel
from utils.events import publish

router = APIRouter(prefix="/api", tags=["Trial Requests"])

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
    class Config: from_attributes = True

@router.get("/trial-requests", response_model=List[TrialRequestOut])
def list_trial_requests(db: Session = Depends(get_db)):
    return (
        db.query(TrialRequestModel)
        .filter(TrialRequestModel.deleted_at.is_(None))
        .order_by(TrialRequestModel.created_at.desc())
        .all()
    )

@router.post("/trial-requests", response_model=TrialRequestOut)
async def create_trial_request(req: TrialRequestIn, db: Session = Depends(get_db)):
    obj = TrialRequestModel(**req.model_dump())
    db.add(obj); db.commit(); db.refresh(obj)
    out = TrialRequestOut.model_validate(obj).model_dump()
    await publish("trial_request", out)
    return obj

@router.delete("/trial-requests/{request_id}")
def delete_trial_request(request_id: int, db: Session = Depends(get_db)):
    trial = (
        db.query(TrialRequestModel)
        .filter(TrialRequestModel.id == request_id, TrialRequestModel.deleted_at.is_(None))
        .first()
    )
    if not trial:
        raise HTTPException(status_code=404, detail="Trial request not found")
    trial.deleted_at = datetime.now(timezone.utc)
    db.commit()
    return {"ok": True, "message": "deleted"}
