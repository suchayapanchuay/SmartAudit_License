# license-server/models/activity_log.py
from sqlalchemy import Column, Integer, String, TIMESTAMP, Text
from database import Base

class ActivityLog(Base):
    __tablename__ = "activity_logs"

    id = Column(Integer, primary_key=True, index=True)
    action = Column(String(255), nullable=False)       # ข้อความกิจกรรม
    meta = Column(Text, nullable=True)                 # เก็บรายละเอียดเพิ่มเติม
    created_at = Column(TIMESTAMP, nullable=True)
