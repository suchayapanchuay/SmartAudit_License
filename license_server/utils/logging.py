# license-server/utils/logging.py
from datetime import datetime
from sqlalchemy.orm import Session
from license_server.models.activity_log import ActivityLog

def log_action(db: Session, action: str, meta: str | None = None):
    obj = ActivityLog(
        action=action,
        meta=meta,
        created_at=datetime.utcnow(),
    )
    db.add(obj)
    db.commit()
