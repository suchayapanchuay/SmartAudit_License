from fastapi import APIRouter, Depends
from sqlalchemy.orm import Session
from database import get_db
from models.license import License
from models.user import User
from models.activity_log import ActivityLog  
from datetime import date
from sqlalchemy import func
from fastapi.responses import JSONResponse

router = APIRouter()

@router.get("/dashboard")
def get_dashboard_data(db: Session = Depends(get_db)):
    today = date.today()

    total_licenses = db.query(func.count(License.id)).scalar()
    active_licenses = db.query(License).filter(License.expire_date >= today).count()
    expired_licenses = db.query(License).filter(License.expire_date < today).count()
    total_users = db.query(func.count(User.id)).scalar()

    raw_chart = (
        db.query(
            func.date_format(License.created_at, '%Y-%m').label("month"),
            func.count(License.id).label("activations")
        )
        .group_by(func.date_format(License.created_at, '%Y-%m'))
        .order_by(func.date_format(License.created_at, '%Y-%m'))
        .all()
    )

    chart_data = [{"month": month, "activations": activations} for month, activations in raw_chart]

    logs = (
        db.query(ActivityLog)
        .order_by(ActivityLog.created_at.desc())
        .limit(10)
        .all()
    )
    
    activity_log = [
        f"{log.created_at.strftime('%Y-%m-%d %H:%M:%S')} - {log.action}" for log in logs
    ]


    return JSONResponse(
        content={
            "total_licenses": total_licenses,
            "active_licenses": active_licenses,
            "expired_licenses": expired_licenses,
            "total_users": total_users,
            "chart_data": chart_data,
            "activity_log": activity_log,
        },
        media_type="application/json; charset=utf-8"
    )
