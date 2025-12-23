# from sqlalchemy.orm import Session
# from typing import Any, Optional

# from models.activity_log import ActivityLog

# def log_activity(
#     db: Session,
#     *,
#     actor: str,
#     action: str,
#     target_type: Optional[str] = None,
#     target_id: Optional[int] = None,
#     message: Optional[str] = None,
#     ip: Optional[str] = None,
#     user_agent: Optional[str] = None,
#     meta: Optional[dict[str, Any]] = None,
# ):
#     row = ActivityLog(
#         actor=actor,
#         action=action,
#         target_type=target_type,
#         target_id=target_id,
#         message=message,
#         ip=ip,
#         user_agent=user_agent,
#         meta_json=meta or {},
#     )
#     db.add(row)

from sqlalchemy.orm import Session
from typing import Optional, Any
from license_server.models.activity_log import ActivityLog

def log_activity(
    db: Session,
    *,
    actor: str,
    action: str,
    target_type: Optional[str] = None,
    target_id: Optional[int] = None,
    message: Optional[str] = None,
    ip: Optional[str] = None,
    user_agent: Optional[str] = None,
    meta: Optional[dict[str, Any]] = None,
):
    row = ActivityLog(
        actor=actor,
        action=action,
        target_type=target_type,
        target_id=target_id,
        message=message,
        ip=ip,
        user_agent=user_agent,
        meta_json=meta or {},
    )
    db.add(row)
    db.commit()        # ❗ สำคัญมาก
    db.refresh(row)
    return row

