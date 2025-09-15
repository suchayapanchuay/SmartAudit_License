from sqlalchemy import Column, Integer, String, TIMESTAMP
from sqlalchemy.sql import func
from database import Base

class Customer(Base):
    __tablename__ = "customers"
    id = Column(Integer, primary_key=True, autoincrement=True)
    email = Column(String(255), unique=True, index=True, nullable=False)
    name = Column(String(255))
    created_at = Column(TIMESTAMP, server_default=func.now())
