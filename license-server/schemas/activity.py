from pydantic import BaseModel, Field
from typing import Any, Optional
from datetime import datetime

class ActivityLogOut(BaseModel):
    id: int
    actor: str
    action: str
    target_type: Optional[str] = None
    target_id: Optional[int] = None
    message: Optional[str] = None
    ip: Optional[str] = None
    user_agent: Optional[str] = None
    meta_json: Optional[dict[str, Any]] = None
    created_at: datetime

    class Config:
        from_attributes = True
