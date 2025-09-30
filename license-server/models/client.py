#from sqlalchemy import Column, Integer, String, TIMESTAMP, Enum as SAEnum
#from database import Base
#
#class Client(Base):
#    __tablename__ = "clients"
#
#    id = Column(Integer, primary_key=True)
#    request_type = Column(SAEnum("trial", "purchase", "support", name="client_request_type", native_enum=False), nullable=False)
#    source = Column(String(32))
#    source_id = Column(String(64))
#
#    first_name = Column(String(100), nullable=False)
#    last_name  = Column(String(100), nullable=False)
#    email      = Column(String(255), nullable=False, unique=True)
#    phone      = Column(String(64))
#    company    = Column(String(255))
#    industry   = Column(String(255))
#    country    = Column(String(128))
#    message    = Column(String(1000))
#    estimate_user = Column(Integer)
#    trial_days    = Column(Integer)
#
#    created_at = Column(TIMESTAMP, nullable=True)

# models/client.py
from sqlalchemy import Column, Integer, String, TIMESTAMP, Enum as SAEnum
from database import Base

class Client(Base):
    __tablename__ = "clients"

    id = Column(Integer, primary_key=True)
    request_type = Column(SAEnum("trial", "purchase", "support", name="client_request_type", native_enum=False), nullable=False)
    source = Column(String(32))
    source_id = Column(String(64))

    first_name = Column(String(100), nullable=False)
    last_name  = Column(String(100), nullable=False)
    email      = Column(String(255), nullable=False, unique=True)
    phone      = Column(String(64))
    company    = Column(String(255))
    industry   = Column(String(255))
    country    = Column(String(128))
    message    = Column(String(1000))
    estimate_user = Column(Integer)
    trial_days    = Column(Integer)

    created_at = Column(TIMESTAMP, nullable=True)



