#from sqlalchemy import Column, Integer, String, DateTime, Enum, ForeignKey
#from database import Base
#from datetime import datetime
#
#class License(Base):
#    __tablename__ = "licenses"
#
#    id = Column(Integer, primary_key=True, index=True)
#    license_key = Column(String(255), unique=True, index=True, nullable=False)
#    product_name = Column(String(255), nullable=False)
#    user_id = Column(Integer, nullable=True)
#    license_type_id = Column(Integer, nullable=True)   # เพิ่มบรรทัดนี้
#    issued_date = Column(DateTime, default=datetime.utcnow)
#    expire_date = Column(DateTime)
#    max_activations = Column(Integer, default=1)
#    activations = Column(Integer, default=0)
#    status = Column(Enum('active', 'revoked', 'expired', name='license_status'), default='active')
#    created_at = Column(DateTime, default=datetime.utcnow)
#    updated_at = Column(DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)
from sqlalchemy import Column, Integer, String, DateTime
from datetime import datetime
from database import Base

class License(Base):
    __tablename__ = "licenses"

    id = Column(Integer, primary_key=True, index=True)
    license_key = Column(String(255), unique=True, index=True, nullable=False)
    product_name = Column(String(255), nullable=False)
    user_id = Column(Integer, nullable=True)
    license_type_id = Column(Integer, nullable=True)
    issued_date = Column(DateTime, default=datetime.utcnow)
    expire_date = Column(DateTime)
    max_activations = Column(Integer, default=1)
    activations = Column(Integer, default=0)
    status = Column(String(20), default="active")
    created_at = Column(DateTime, default=datetime.utcnow)
    updated_at = Column(DateTime, default=datetime.utcnow)
