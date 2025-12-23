#from sqlalchemy import Column, Integer, String, TIMESTAMP, ForeignKey
#from sqlalchemy.orm import relationship
#from database import Base
#
#class License(Base):
#    __tablename__ = "licenses"
#
#    id = Column(Integer, primary_key=True)
#    client_id = Column(Integer, ForeignKey("clients.id", ondelete="CASCADE"), nullable=False)
#
#    license_key = Column(String(64), unique=True, nullable=False, index=True)
#
#    # ไม่ผูก ENUM เพื่อให้ทำงานได้ทั้ง MariaDB/SQLite
#    term = Column(String(32), nullable=False, default="trial")         # trial | subscription | perpetual
#    product_sku = Column(String(64), nullable=True)
#
#    duration_days = Column(Integer, nullable=True)                     # ใช้กับ trial/subscription
#    max_activations = Column(Integer, nullable=False, default=1)
#    activations_used = Column(Integer, nullable=False, default=0)
#
#    status = Column(String(16), nullable=False, default="active")      # active | revoked | expired
#
#    issued_at = Column(TIMESTAMP, nullable=True)
#    expires_at = Column(TIMESTAMP, nullable=True)
#    created_at = Column(TIMESTAMP, nullable=True)
#
#    client = relationship("Client", backref="licenses")

# models/license.py
from sqlalchemy import Column, Integer, String, TIMESTAMP, ForeignKey
from sqlalchemy.orm import relationship
from license_server.database import Base

class License(Base):
    __tablename__ = "licenses"

    id = Column(Integer, primary_key=True)
    client_id = Column(Integer, ForeignKey("clients.id", ondelete="CASCADE"), nullable=False)

    license_key = Column(String(64), unique=True, nullable=False, index=True)

    term = Column(String(32), nullable=False, default="trial")   # trial | subscription | perpetual
    product_sku = Column(String(64), nullable=True)

    duration_days = Column(Integer, nullable=True)
    max_activations = Column(Integer, nullable=False, default=1)
    activations_used = Column(Integer, nullable=False, default=0)

    status = Column(String(16), nullable=False, default="active")  # active | revoked | expired

    issued_at = Column(TIMESTAMP, nullable=True)
    expires_at = Column(TIMESTAMP, nullable=True)
    created_at = Column(TIMESTAMP, nullable=True)

    client = relationship("Client", backref="licenses")
