#from sqlalchemy import Column, Integer, String, Enum, TIMESTAMP
#from sqlalchemy.sql import func
#from database import Base
#
#class Product(Base):
#    __tablename__ = "products"
#    id = Column(Integer, primary_key=True, autoincrement=True)
#    sku = Column(String(64), unique=True, index=True, nullable=False)
#    name = Column(String(255), nullable=False)
#    term = Column(Enum('perpetual', 'subscription'), nullable=False, default='subscription')
#    duration_months = Column(Integer)
#    max_activations = Column(Integer, nullable=False, default=1)
#    created_at = Column(TIMESTAMP, server_default=func.now())

from sqlalchemy import Column, Integer, String, TIMESTAMP
from sqlalchemy.dialects.mysql import ENUM as MYSQL_ENUM
from database import Base

class Product(Base):
    __tablename__ = "products"
    id = Column(Integer, primary_key=True)
    sku = Column(String(64), nullable=False, unique=True)
    name = Column(String(255), nullable=False)
    term = Column(MYSQL_ENUM('perpetual','subscription'), nullable=False, default="subscription")
    duration_months = Column(Integer)
    max_activations = Column(Integer, nullable=False, default=1)
    created_at = Column(TIMESTAMP, nullable=True)

