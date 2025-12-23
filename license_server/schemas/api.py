from pydantic import BaseModel, Field
from typing import List, Optional
from datetime import datetime
from license_server.models.api_key import KeyStatus

class ApiKeyCreateIn(BaseModel):
    name: str = Field(min_length=1)
    scopes: List[str] = Field(default_factory=list)
    status: KeyStatus = KeyStatus.active
    expires_in_days: Optional[int] = None

class ApiKeyCreatedOut(BaseModel):
    id: int
    name: str
    scopes: List[str]
    status: KeyStatus
    expires_at: datetime | None
    plaintext_key: str
    mask: str

class ApiKeyListItem(BaseModel):
    id: int
    name: str
    key_mask: str
    scopes: List[str]
    status: KeyStatus
    lastUsed: str | None
    expiresAt: str | None
