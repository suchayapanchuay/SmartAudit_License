from datetime import datetime
from typing import Any, Dict, List, Optional

from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel, EmailStr
from sqlalchemy.orm import Session

from license_server.database import get_db
from license_server.models.trial_request import TrialRequest
from license_server.utils.events import publish

router = APIRouter(prefix="/api", tags=["trial-requests"])

# ---- Schemas (รับ camelCase จากฟรอนต์) ----
class UTMModel(BaseModel):
    # เก็บอะไรก็ได้ → map เป็น string JSON ตอนบันทึก
    model_config = {"extra": "allow"}

class TrialIn(BaseModel):
    firstName: str
    lastName: str
    email: EmailStr
    phone: Optional[str] = None
    company: str
    industry: Optional[str] = None
    country: str
    jobTitle: Optional[str] = None
    message: Optional[str] = None
    utm: Optional[Dict[str, Any]] = None

class TrialOut(BaseModel):
    id: int
    firstName: str
    lastName: str
    email: EmailStr
    phone: Optional[str]
    company: str
    industry: Optional[str]
    country: str
    jobTitle: Optional[str]
    message: Optional[str]
    created_at: Optional[str]

def _to_out(o: TrialRequest) -> TrialOut:
    return TrialOut(
        id=o.id,
        firstName=o.first_name,
        lastName=o.last_name,
        email=o.email,
        phone=o.phone,
        company=o.company,
        industry=o.industry,
        country=o.country,
        jobTitle=o.job_title,
        message=o.message,
        created_at=o.created_at.isoformat() if o.created_at else None
    )

@router.post("/trial-requests", response_model=TrialOut)
def create_trial_request(body: TrialIn, db: Session = Depends(get_db)):
    import json
    obj = TrialRequest(
        first_name=body.firstName.strip(),
        last_name=body.lastName.strip(),
        email=str(body.email).strip(),
        phone=body.phone,
        company=body.company.strip(),
        industry=(body.industry or None),
        country=body.country.strip(),
        job_title=(body.jobTitle or None),
        message=(body.message or None),
        utm=json.dumps(body.utm or {}, ensure_ascii=False),
        created_at=datetime.utcnow(),
    )
    db.add(obj)
    db.commit()
    db.refresh(obj)

    publish("trial_request_created", {
        "id": obj.id,
        "first_name": obj.first_name,
        "last_name": obj.last_name,
        "email": obj.email,
        "phone": obj.phone,
        "company": obj.company,
        "industry": obj.industry,
        "country": obj.country,
        "message": obj.message,
        "created_at": obj.created_at.isoformat() if obj.created_at else None,
    })

    return _to_out(obj)

@router.get("/trial-requests", response_model=List[TrialOut])
def list_trial_requests(db: Session = Depends(get_db)):
    q = db.query(TrialRequest).order_by(TrialRequest.id.desc()).all()
    return [_to_out(x) for x in q]

@router.delete("/trial-requests/{trial_id}")
def delete_trial_request(trial_id: int, db: Session = Depends(get_db)):
    obj = db.get(TrialRequest, trial_id)
    if not obj:
        raise HTTPException(status_code=404, detail="Not found")
    db.delete(obj)
    db.commit()
    return {"ok": True}

