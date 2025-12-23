# models/product.py
from sqlalchemy import Column, Integer, String, Text, Boolean, TIMESTAMP
from sqlalchemy.dialects.mysql import JSON as MYSQL_JSON
from license_server.database import Base

class Product(Base):
    __tablename__ = "products"

    id = Column(Integer, primary_key=True)
    sku = Column(String(64), nullable=True, unique=True)        # optional
    name = Column(String(255), nullable=False, unique=True)
    category = Column(String(100), nullable=True)
    is_active = Column(Boolean, nullable=False, default=True)
    description = Column(Text, nullable=True)
    meta = Column(MYSQL_JSON, nullable=True)                    # version / licensePolicy / constraints
    created_at = Column(TIMESTAMP, nullable=True)
