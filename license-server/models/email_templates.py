# models/email_templates.py
from sqlalchemy import Column, String, Text, Boolean, DateTime
from datetime import datetime
from database import Base

class EmailTemplate(Base):
    __tablename__ = "email_templates"

    id = Column(String(36), primary_key=True, index=True)   # UUID
    slug = Column(String(64), unique=True, nullable=False)
    name = Column(String(255), nullable=False)
    subject = Column(String(255), nullable=False)
    body = Column(Text, nullable=False)

    is_html = Column(Boolean, default=True, nullable=False)
    status = Column(String(20), default="Active", nullable=False)

    created_at = Column(DateTime, default=datetime.utcnow, nullable=False)
    updated_at = Column(DateTime, default=None, onupdate=datetime.utcnow)
