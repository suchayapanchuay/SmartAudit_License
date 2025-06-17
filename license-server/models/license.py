from sqlalchemy import Column, Integer, String, DateTime, Boolean
from datetime import datetime
from database import Base

class License(Base):
    __tablename__ = "licenses"

    id = Column(Integer, primary_key=True)
    license_key = Column(String(255), unique=True, index=True)
    product = Column(String(50))
    valid_until = Column(DateTime)
    active = Column(Boolean, default=True)
    created_at = Column(DateTime, default=datetime.utcnow)