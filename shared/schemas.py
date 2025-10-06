# from pydantic import BaseModel, EmailStr, Field
# from typing import Optional, Dict, Any

# # === ฟอร์ม Free Trial ===
# class TrialRequestIn(BaseModel):
#     type: str = Field(default="trial_request")
#     firstName: str
#     lastName: str
#     email: EmailStr
#     phone: str
#     company: str
#     industry: str
#     country: str
#     jobTitle: str
#     utm: Optional[Dict[str, Any]] = None

# class TrialRequestAck(BaseModel):
#     ok: bool = True
#     message: str = "received"

# # === อีเวนต์ Order สำหรับแจ้งเตือน ===
# class OrderEventIn(BaseModel):
#     type: str = Field(default="order_created")
#     order_code: str
#     email: EmailStr
#     name: str
#     meta: Optional[Dict[str, Any]] = None

# class OkResp(BaseModel):
#     ok: bool = True
#     message: str = "ok"

# schemas.py (หรือส่วนเดียวกับ router)
from pydantic import BaseModel, Field
from typing import Optional, List, Literal
from datetime import datetime

Status = Literal["Active", "Draft", "Disabled"]

class EmailTemplateIn(BaseModel):
    slug: str = Field(..., min_length=2, max_length=64)
    name: str = Field(..., min_length=1, max_length=128)
    subject: str = Field(..., min_length=1, max_length=256)
    body: str
    status: Status = "Active"
    is_html: bool = False

class EmailTemplatePatch(BaseModel):
    # ไม่เปิดให้แก้ slug
    name: Optional[str] = Field(None, min_length=1, max_length=128)
    subject: Optional[str] = Field(None, min_length=1, max_length=256)
    body: Optional[str] = None
    status: Optional[Status] = None
    is_html: Optional[bool] = None

class EmailTemplateOut(BaseModel):
    id: int
    slug: str
    name: str
    subject: str
    body: str
    status: Status
    is_html: bool
    created_at: Optional[datetime] = None
    updated_at: Optional[datetime] = None

    class Config:
        orm_mode = True  # ถ้า Pydantic v2: model_config = {"from_attributes": True}

