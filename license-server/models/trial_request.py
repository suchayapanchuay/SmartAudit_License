from sqlalchemy import Column, Integer, String, DateTime, Text, func
from sqlalchemy.dialects.mysql import JSON as MyJSON
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
    utm = Column(MyJSON, nullable=True)

    created_at = Column(DateTime, server_default=func.now(), nullable=False, index=True)
    deleted_at = Column(DateTime, nullable=True, index=True)
