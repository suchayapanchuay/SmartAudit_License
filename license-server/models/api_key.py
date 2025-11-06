from sqlalchemy import String, JSON, Enum, DateTime, Integer
from sqlalchemy.orm import Mapped, mapped_column
from datetime import datetime
from database import Base
import enum

class KeyStatus(str, enum.Enum):
    active = "active"
    inactive = "inactive"
    revoked = "revoked"

class ApiKey(Base):
    __tablename__ = "api_keys"
    id: Mapped[int] = mapped_column(primary_key=True, autoincrement=True)
    name: Mapped[str] = mapped_column(String(255), nullable=False)
    key_prefix: Mapped[str] = mapped_column(String(16), nullable=False)
    key_last4: Mapped[str] = mapped_column(String(4), nullable=False)
    key_hash: Mapped[str] = mapped_column(String(64), nullable=False)  # SHA-256 hex
    scopes_json: Mapped[dict] = mapped_column(JSON, nullable=False)    # {"scopes": ["issue_license",...]}
    status: Mapped[KeyStatus] = mapped_column(Enum(KeyStatus), default=KeyStatus.active, nullable=False)
    expires_at: Mapped[datetime | None] = mapped_column(DateTime, nullable=True)
    last_used_at: Mapped[datetime | None] = mapped_column(DateTime, nullable=True)
    created_by: Mapped[str | None] = mapped_column(String(255), nullable=True)
    created_at: Mapped[datetime] = mapped_column(DateTime, default=datetime.utcnow, nullable=False)

class ApiKeyUsage(Base):
    __tablename__ = "api_key_usage"
    id: Mapped[int] = mapped_column(primary_key=True, autoincrement=True)
    api_key_id: Mapped[int] = mapped_column(Integer, nullable=False)
    method: Mapped[str] = mapped_column(String(8), nullable=False)
    path: Mapped[str] = mapped_column(String(512), nullable=False)
    status_code: Mapped[int] = mapped_column(Integer, nullable=False)
    ip_addr: Mapped[str | None] = mapped_column(String(64), nullable=True)
    used_at: Mapped[datetime] = mapped_column(DateTime, default=datetime.utcnow, nullable=False)
