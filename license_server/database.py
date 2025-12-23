#import os
#from sqlalchemy import create_engine
#from sqlalchemy.orm import sessionmaker, declarative_base
#
#DATABASE_URL = os.getenv(
#    "DATABASE_URL",
#    "mysql+pymysql://smartaudit_user:alealeale123@127.0.0.1:3306/smartaudit"
#)
#
#engine = create_engine(DATABASE_URL, pool_pre_ping=True)
#
## กันพลาด: ENUM จะพังถ้าใช้ sqlite
#if engine.url.get_backend_name() not in {"mysql", "mariadb"}:
#    raise RuntimeError(
#        f"DATABASE_URL ใช้ '{engine.url.get_backend_name()}' อยู่! "
#        "กรุณาตั้งเป็น MariaDB/MySQL เช่น "
#        "mysql+pymysql://user:pass@host:3306/smartaudit"
#    )
#
#Base = declarative_base()
#SessionLocal = sessionmaker(bind=engine, autoflush=False, autocommit=False)
#
#def get_db():
#    db = SessionLocal()
#    try:
#        yield db
#    finally:
#        db.close()

import os
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker, declarative_base

# แก้ค่าให้ตรงกับเครื่อง/Compose ของคุณ
DATABASE_URL = os.getenv(
    "DATABASE_URL",
    "mysql+pymysql://smartaudit_user:alealeale123@127.0.0.1:3306/smartaudit"
)

engine = create_engine(DATABASE_URL, pool_pre_ping=True)

# ป้องกันการเผลอใช้ sqlite (จะพังกับ ENUM)
if engine.url.get_backend_name() not in {"mysql", "mariadb"}:
    raise RuntimeError(
        f"DATABASE_URL backend '{engine.url.get_backend_name()}' is not MySQL/MariaDB. "
        "Please use MariaDB/MySQL (pymysql)."
    )

Base = declarative_base()
SessionLocal = sessionmaker(bind=engine, autoflush=False, autocommit=False)

def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()


