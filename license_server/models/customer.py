# from sqlalchemy import Column, Integer, String, TIMESTAMP
# from database import Base
# 
# class Customer(Base):
    # __tablename__ = "customers"
# 
    # id = Column(Integer, primary_key=True)
    # email = Column(String(255), nullable=False, unique=True)
    # name  = Column(String(255))
    # created_at = Column(TIMESTAMP, nullable=True)

from sqlalchemy import Column, Integer, String, TIMESTAMP
from license_server.database import Base

class Customer(Base):
    __tablename__ = "customers"

    id = Column(Integer, primary_key=True)
    email = Column(String(255), nullable=False, unique=True)
    name = Column(String(255), nullable=True)
    created_at = Column(TIMESTAMP, nullable=True)


