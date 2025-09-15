#from fastapi import APIRouter
#from pydantic import BaseModel, EmailStr, Field
#from utils.events import publish
#
#router = APIRouter()
#
#class TrialRequest(BaseModel):
#    firstName: str = Field(min_length=1)
#    lastName: str = Field(min_length=1)
#    email: EmailStr
#    phone: str
#    company: str
#    industry: str
#    country: str
#    jobTitle: str
#    message: str | None = None
#    utm: dict | None = None
#
#@router.get("/trial-requests")
#async def trial_requests_info():
#    return {
#        "ok": True,
#        "message": "Use POST /trial-requests to submit a trial request.",
#    }
#
#@router.post("/trial-requests")
#async def create_trial_request(req: TrialRequest):
#    # TODO: บันทึก DB หากต้องการ
#    await publish("trial_request", req.model_dump())
#    return {"ok": True, "message": "received"}
#
## สำหรับยิงทดสอบแบบ GET ให้ noti เด้ง
#@router.get("/trial-requests/_test-fire")
#async def test_fire():
#    demo = {
#        "firstName": "Test",
#        "lastName": "User",
#        "email": "test@example.com",
#        "phone": "000",
#        "company": "Demo Co",
#        "industry": "tech",
#        "country": "th",
#        "jobTitle": "dev",
#        "message": "hello from GET test",
#    }
#    await publish("trial_request", demo)
#    return {"ok": True, "fired": demo}

# license-server/models/trial_request.py
from sqlalchemy import Column, Integer, String, DateTime, Text, func
from sqlalchemy.dialects.mysql import JSON as MyJSON  # MariaDB/MySQL JSON (ใน MariaDB เป็น alias เป็น LONGTEXT)
from database import Base

class TrialRequest(Base):
    __tablename__ = "trial_requests"

    id = Column(Integer, primary_key=True, autoincrement=True)

    firstName = Column(String(100), nullable=False)
    lastName = Column(String(100), nullable=False)
    email = Column(String(255), nullable=False, index=True)
    phone = Column(String(100), nullable=False)
    company = Column(String(255), nullable=False)
    industry = Column(String(100), nullable=False)
    country = Column(String(100), nullable=False)
    jobTitle = Column(String(100), nullable=False)

    message = Column(Text, nullable=True)
    utm = Column(MyJSON, nullable=True)  # ถ้ากังวล compatibility ใช้ Text ก็ได้

    created_at = Column(DateTime, server_default=func.now(), nullable=False, index=True)
    deleted_at = Column(DateTime, nullable=True, index=True)

