from sqlalchemy import Column, Integer, String, TIMESTAMP, Text
from license_server.database import Base

class TrialRequest(Base):
    __tablename__ = "trial_requests"

    id = Column(Integer, primary_key=True)
    first_name = Column(String(100), nullable=False)
    last_name  = Column(String(100), nullable=False)
    email      = Column(String(255), nullable=False)
    phone      = Column(String(64))
    company    = Column(String(255), nullable=False)
    industry   = Column(String(255))
    country    = Column(String(128), nullable=False)
    job_title  = Column(String(255))
    message    = Column(String(1000))
    utm        = Column(Text)           # เก็บ JSON เป็นสตริง
    created_at = Column(TIMESTAMP, nullable=True)  # default ให้ DB ใส่เอง
