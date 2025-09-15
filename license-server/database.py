#from sqlalchemy import create_engine
#from sqlalchemy.orm import sessionmaker, declarative_base
#from dotenv import load_dotenv
#import os
#
#load_dotenv()
#DATABASE_URL = os.getenv("DATABASE_URL")
#
#engine = create_engine(DATABASE_URL, pool_pre_ping=True)
#SessionLocal = sessionmaker(bind=engine, autocommit=False, autoflush=False)
#Base = declarative_base()
#
#def get_db():
#    db = SessionLocal()
#    try:
#        yield db
#    finally:
#        db.close()

from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker, declarative_base
from dotenv import load_dotenv
import os

load_dotenv()
DATABASE_URL = os.getenv("DATABASE_URL")  # eg: mysql+pymysql://user:pass@127.0.0.1:3306/smartaudit

engine = create_engine(
    DATABASE_URL,
    pool_pre_ping=True,
    pool_recycle=3600,
    future=True,
    # echo=True,  # เปิดเมื่ออยากเห็น SQL log
)

SessionLocal = sessionmaker(bind=engine, autocommit=False, autoflush=False)
Base = declarative_base()

# absolute imports เพื่อให้ทำงานกับ 'uvicorn main:app'
from models.license import License       # noqa: F401  (ของเดิมคุณ)
from models.user import User             # noqa: F401  (ของเดิมคุณ)
from models.product import Product       # noqa: F401
from models.customer import Customer     # noqa: F401
from models.order import Order           # noqa: F401

def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()
