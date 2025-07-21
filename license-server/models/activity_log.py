# models/activity_log.py

from sqlalchemy import Column, Integer, String, DateTime
from datetime import datetime
from database import Base

class ActivityLog(Base):
    __tablename__ = "activity_logs"

    id = Column(Integer, primary_key=True, index=True)
    action = Column(String(255), nullable=False)
    created_at = Column(DateTime, default=datetime.utcnow)  

