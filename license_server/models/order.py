# models/order.py
from sqlalchemy import Column, Integer, String, TIMESTAMP, ForeignKey, Enum as SAEnum, Text
from license_server.database import Base


class Order(Base):
    __tablename__ = "orders"

    id = Column(Integer, primary_key=True)
    order_code = Column(String(64), nullable=False, unique=True)
    customer_id = Column(Integer, ForeignKey("customers.id"), nullable=False)
    product_id  = Column(Integer, ForeignKey("products.id"), nullable=False)
    amount_cents = Column(Integer, nullable=False, default=0)
    currency = Column(String(8), default="THB")
    status = Column(SAEnum("pending", "paid", "failed", "cancelled",
                           name="order_status", native_enum=False),
                    nullable=False, default="pending")
    meta = Column(Text)  # JSON string
    created_at = Column(TIMESTAMP, nullable=True)


# from sqlalchemy import Column, Integer, String, TIMESTAMP, ForeignKey, Enum as SAEnum, Text
# from database import Base
# 
# class Order(Base):
    # __tablename__ = "orders"
# 
    # id = Column(Integer, primary_key=True)
    # order_code = Column(String(64), nullable=False, unique=True)
    # customer_id = Column(Integer, ForeignKey("customers.id"), nullable=False)
    # product_id  = Column(Integer, ForeignKey("products.id"), nullable=False)
    # amount_cents = Column(Integer, nullable=False, default=0)
    # currency = Column(String(8), default="THB")
    # status = Column(SAEnum("pending", "paid", "failed", "cancelled", name="order_status", native_enum=False), nullable=False, default="pending")
    # meta = Column(Text)  # JSON string
    # created_at = Column(TIMESTAMP, nullable=True)


