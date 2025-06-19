#from sqlalchemy import Column, Integer, String, DateTime, Boolean
#from datetime import datetime
#from database import Base
#
#class License(Base):
#    __tablename__ = "licenses"
#
#    id = Column(Integer, primary_key=True)
#    license_key = Column(String(255), unique=True, index=True)
#    product = Column(String(50))
#    valid_until = Column(DateTime)
#    active = Column(Boolean, default=True)
#    created_at = Column(DateTime, default=datetime.utcnow)
    
from sqlalchemy import Column, Integer, String, DateTime, Boolean, Enum
from database import Base
from datetime import datetime

class License(Base):
    __tablename__ = "licenses"

    id = Column(Integer, primary_key=True, index=True)
    license_key = Column(String(255), unique=True, index=True)
    product_name = Column(String(255))  # ชื่อเดิมในฐานข้อมูล
    user_id = Column(Integer, nullable=True)
    issued_date = Column(DateTime)
    expire_date = Column(DateTime)  # ชื่อเดิมในฐานข้อมูล
    max_activations = Column(Integer, default=1)
    activations = Column(Integer, default=0)
    status = Column(Enum('active', 'revoked', 'expired', name='license_status'), default='active')  # ชื่อเดิมในฐานข้อมูล
    created_at = Column(DateTime, default=datetime.utcnow)
    updated_at = Column(DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)