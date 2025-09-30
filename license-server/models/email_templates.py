from sqlalchemy import Column, Integer, String, Text, DateTime, func, Boolean
from database import Base

class EmailTemplate(Base):
    __tablename__ = "email_templates"

    id = Column(Integer, primary_key=True, index=True)
    slug = Column(String(64), unique=True, index=True, nullable=False)  # เช่น "welcome", "license-expiry"
    name = Column(String(128), nullable=False)
    subject = Column(String(256), nullable=False)
    body = Column(Text, nullable=False)  # รองรับ {{variables}}
    status = Column(String(16), default="Active")  # Active | Draft | Disabled
    created_at = Column(DateTime, server_default=func.now())
    updated_at = Column(DateTime, server_default=func.now(), onupdate=func.now())
    is_html = Column(Boolean, default=False)  # ถ้าอยากส่ง HTML ตรง ๆ
