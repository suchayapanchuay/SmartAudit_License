from fastapi import APIRouter, Depends, Query
from sqlalchemy.orm import Session
from sqlalchemy import and_
from datetime import datetime
from typing import Optional

from database import get_db
from models.activity_log import ActivityLog
from schemas.activity import ActivityLogOut

router = APIRouter(prefix="/api/admin/activity-logs", tags=["admin-activity-logs"])

def admin_required():
    return True

@router.get("", response_model=list[ActivityLogOut])
def list_activity_logs(
    db: Session = Depends(get_db),
    _: bool = Depends(admin_required),
    user: Optional[str] = Query(default=None, description="กรองด้วย actor"),
    action: Optional[str] = Query(default=None, description="กรองด้วย action"),
    since: Optional[str] = Query(default=None, description="ISO datetime จากวันที่นี้เป็นต้นไป"),
    until: Optional[str] = Query(default=None, description="ISO datetime จนถึงวันนี้"),
    page: int = Query(default=1, ge=1),
    page_size: int = Query(default=100, ge=1, le=500),
):
    q = db.query(ActivityLog)

    if user:
        q = q.filter(ActivityLog.actor == user)
    if action:
        q = q.filter(ActivityLog.action == action)

    # parse ISO times ถ้าส่งมา
    def parse_iso(s: Optional[str]) -> Optional[datetime]:
        if not s:
            return None
        try:
            return datetime.fromisoformat(s.replace("Z", "+00:00")).replace(tzinfo=None)
        except Exception:
            return None

    since_dt = parse_iso(since)
    until_dt = parse_iso(until)
    if since_dt and until_dt:
        q = q.filter(and_(ActivityLog.created_at >= since_dt, ActivityLog.created_at <= until_dt))
    elif since_dt:
        q = q.filter(ActivityLog.created_at >= since_dt)
    elif until_dt:
        q = q.filter(ActivityLog.created_at <= until_dt)

    q = q.order_by(ActivityLog.created_at.desc())
    rows = q.offset((page - 1) * page_size).limit(page_size).all()
    return rows
