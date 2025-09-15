# shared/schemas.py
from pydantic import BaseModel, EmailStr, Field
from typing import Optional, Dict, Any

# === ฟอร์ม Free Trial ===
class TrialRequestIn(BaseModel):
    type: str = Field(default="trial_request")
    firstName: str
    lastName: str
    email: EmailStr
    phone: str
    company: str
    industry: str
    country: str
    jobTitle: str
    utm: Optional[Dict[str, Any]] = None

class TrialRequestAck(BaseModel):
    ok: bool = True
    message: str = "received"

# === อีเวนต์ Order สำหรับแจ้งเตือน ===
class OrderEventIn(BaseModel):
    type: str = Field(default="order_created")
    order_code: str
    email: EmailStr
    name: str
    meta: Optional[Dict[str, Any]] = None

class OkResp(BaseModel):
    ok: bool = True
    message: str = "ok"
