from sqlalchemy import Column, Integer, String, DateTime, JSON
from datetime import datetime
from database import Base

class ActivityLog(Base):
    __tablename__ = "activity_logs"

    id          = Column(Integer, primary_key=True, index=True)
    actor       = Column(String(255), nullable=False)
    action      = Column(String(64),  nullable=False)   # e.g., "api_key.created"
    target_type = Column(String(64),  nullable=True)    # e.g., "api_key"
    target_id   = Column(Integer,     nullable=True)
    message     = Column(String(512), nullable=True)
    ip          = Column(String(64),  nullable=True)
    user_agent  = Column(String(255), nullable=True)
    meta_json   = Column(JSON,        nullable=True)
    created_at  = Column(DateTime,    nullable=False, default=datetime.utcnow)
