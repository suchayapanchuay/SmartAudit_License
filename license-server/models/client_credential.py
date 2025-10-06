from sqlalchemy import Column, Integer, String, DateTime, ForeignKey, func
from sqlalchemy.orm import relationship
from database import Base

# class ClientCredential(Base):
#     __tablename__ = "client_credentials"

#     id = Column(Integer, primary_key=True, index=True)
#     client_id = Column(Integer, ForeignKey("clients.id"), index=True, nullable=False)
#     username = Column(String(128), unique=False, index=True, nullable=False)
#     password_hash = Column(String(255), nullable=False)
#     created_at = Column(DateTime, server_default=func.now())

#     client = relationship("Client", back_populates="credential")  # optional

class ClientCredential(Base):
    __tablename__ = "client_credentials"

    id = Column(Integer, primary_key=True, index=True)
    client_id = Column(Integer, ForeignKey("clients.id"), index=True, nullable=False)
    username = Column(String(128), unique=True, index=True, nullable=False)  # <-- แก้ unique=True
    password_hash = Column(String(255), nullable=False)
    created_at = Column(DateTime, server_default=func.now())

    client = relationship("Client", back_populates="credential")
