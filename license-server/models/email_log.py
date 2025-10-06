from sqlalchemy import Column, Integer, String, DateTime, Text
from sqlalchemy.sql import func
from database import Base

class EmailLog(Base):
    __tablename__ = "email_logs"

    id = Column(Integer, primary_key=True)
    client_id = Column(Integer, index=True, nullable=True)
    to_email = Column(String(255), index=True)
    subject = Column(String(255))
    template_slug = Column(String(100))
    status = Column(String(30), default="queued")  # queued|sent|failed
    error = Column(Text, nullable=True)
    created_at = Column(DateTime(timezone=True), server_default=func.now())
