#from sqlalchemy import Column, Integer, String, DateTime, func
#from sqlalchemy.orm import relationship
#from database import Base
#
#class Customer(Base):
#    __tablename__ = "customers"
#
#    id = Column(Integer, primary_key=True)
#    name = Column(String(255), nullable=False)
#    email = Column(String(255), nullable=False, index=True)
#    company = Column(String(255), nullable=True)
#    phone = Column(String(50), nullable=True)
#    created_at = Column(DateTime(timezone=True), server_default=func.now())
#
#    orders = relationship("Order", back_populates="customer")

from sqlalchemy import Column, Integer, String, TIMESTAMP
from database import Base

class Customer(Base):
    __tablename__ = "customers"

    id = Column(Integer, primary_key=True)
    email = Column(String(255), nullable=False, unique=True)
    name  = Column(String(255))
    created_at = Column(TIMESTAMP, nullable=True)

