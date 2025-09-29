#from sqlalchemy import Column, Integer, String, DateTime, Enum, JSON, Numeric, ForeignKey, func
#from sqlalchemy.orm import relationship
#from database import Base
#import enum
#
#class OrderStatus(str, enum.Enum):
#    pending = "pending"
#    paid = "paid"
#    processing = "processing"
#    completed = "completed"
#    cancelled = "cancelled"
#
#class Order(Base):
#    __tablename__ = "orders"
#
#    id = Column(Integer, primary_key=True, index=True)
#
#    customer_id = Column(Integer, ForeignKey("customers.id"), nullable=True)
#    customer_name = Column(String(255), nullable=False)
#    customer_email = Column(String(255), nullable=False, index=True)
#    company = Column(String(255), nullable=True)
#    phone = Column(String(50), nullable=True)
#
#    form_type = Column(String(50), nullable=False, default="Request")
#
#    items = Column(JSON, nullable=False, default=[])
#    grand_total = Column(Numeric(12, 2), nullable=True)
#    note = Column(String(2000), nullable=True)
#
#    status = Column(Enum(OrderStatus), nullable=False, default=OrderStatus.pending)
#
#    created_at = Column(DateTime(timezone=True), server_default=func.now())
#    updated_at = Column(DateTime(timezone=True), onupdate=func.now())
#    deleted_at = Column(DateTime(timezone=True), nullable=True)
#
#    customer = relationship("Customer", back_populates="orders", lazy="joined", uselist=False)

from sqlalchemy import Column, Integer, String, TIMESTAMP, ForeignKey, Enum as SAEnum, Text
from database import Base

class Order(Base):
    __tablename__ = "orders"

    id = Column(Integer, primary_key=True)
    order_code = Column(String(64), nullable=False, unique=True)
    customer_id = Column(Integer, ForeignKey("customers.id"), nullable=False)
    product_id  = Column(Integer, ForeignKey("products.id"), nullable=False)
    amount_cents = Column(Integer, nullable=False, default=0)
    currency = Column(String(8), default="THB")
    status = Column(SAEnum("pending", "paid", "failed", "cancelled", name="order_status", native_enum=False), nullable=False, default="pending")
    meta = Column(Text)  # JSON string
    created_at = Column(TIMESTAMP, nullable=True)


