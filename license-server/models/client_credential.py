from sqlalchemy import Column, Integer, String, TIMESTAMP, ForeignKey
from database import Base

class ClientCredential(Base):
    __tablename__ = "client_credentials"
    id = Column(Integer, primary_key=True)
    client_id = Column(Integer, ForeignKey("clients.id", ondelete="CASCADE"), nullable=False)
    username = Column(String(100), nullable=False, unique=True)
    password_hash = Column(String(255), nullable=False)
    created_at = Column(TIMESTAMP, nullable=True)

