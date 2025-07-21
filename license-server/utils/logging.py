# utils/logging.py

from models.activity_log import ActivityLog
from sqlalchemy.orm import Session

def log_action(db: Session, action: str):
    log = ActivityLog(action=action)
    db.add(log)
    db.commit()

