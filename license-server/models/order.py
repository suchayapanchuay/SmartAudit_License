from sqlalchemy import Column, Integer, String, ForeignKey, Enum, TIMESTAMP, Text
from sqlalchemy.sql import func
from sqlalchemy.orm import relationship
from database import Base

class Order(Base):
    __tablename__ = "orders"
    id = Column(Integer, primary_key=True, autoincrement=True)
    order_code = Column(String(64), unique=True, index=True, nullable=False)
    customer_id = Column(Integer, ForeignKey("customers.id"), nullable=False)
    product_id = Column(Integer, ForeignKey("products.id"), nullable=False)
    amount_cents = Column(Integer, nullable=False, default=0)
    currency = Column(String(8), default='THB')
    status = Column(Enum('pending','paid','failed','cancelled'), default='pending', nullable=False)
    meta = Column(Text)  # ใช้ Text เพื่อความเข้ากันได้กว้าง
    created_at = Column(TIMESTAMP, server_default=func.now())

    customer = relationship("Customer")
    product = relationship("Product")
