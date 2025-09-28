# models/trial_request.py
from sqlalchemy import Column, Integer, String, DateTime, JSON, func
from database import Base

class TrialRequest(Base):
    __tablename__ = "trial_requests"

    id = Column(Integer, primary_key=True, index=True)
    firstName = Column(String(100), nullable=False)
    lastName  = Column(String(100), nullable=False)
    email     = Column(String(255), nullable=False, index=True)
    phone     = Column(String(50), nullable=False)
    company   = Column(String(255), nullable=False)
    industry  = Column(String(100), nullable=False)
    country   = Column(String(100), nullable=False)
    jobTitle  = Column(String(100), nullable=False)
    message   = Column(String(2000), nullable=True)
    utm       = Column(JSON, nullable=True)
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    deleted_at = Column(DateTime(timezone=True), nullable=True)
