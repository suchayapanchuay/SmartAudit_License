# routes/dashboard_route.py
from __future__ import annotations

from datetime import datetime, timedelta, date
from fastapi import APIRouter, Depends
from sqlalchemy import func, and_, case, or_, cast, Date
from sqlalchemy.orm import Session

from database import SessionLocal
import models  # ควรมีอย่างน้อย License, (Client/ActivityLog/TrialRequest มีหรือไม่ก็ได้)

router = APIRouter(prefix="/api/dashboard", tags=["dashboard"])

# ---------- DB Session ----------
def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()

# ---------- Helpers ----------
def today_utc_date() -> date:
    return datetime.utcnow().date()

def get_expiry_col(License):
    """คืนคอลัมน์วันหมดอายุ (รองรับหลายชื่อ)"""
    return (
        getattr(License, "expiry_date", None)
        or getattr(License, "expiry_at", None)
        or getattr(License, "expires_at", None)
        or getattr(License, "expire_date", None)
    )

def build_is_trial_pred(License):
    """บอกว่าเป็น Trial หรือไม่ (ยืดหยุ่นหลาย schema)"""
    preds = []
    if hasattr(License, "is_trial"):
        preds.append(getattr(License, "is_trial") == True)  # noqa: E712
    for colname in ("term", "type", "plan", "license_type"):
        if hasattr(License, colname):
            preds.append(func.lower(getattr(License, colname)) == "trial")
    return or_(*preds) if preds else None


@router.get("/metrics")
def get_metrics(db: Session = Depends(get_db)):
    """
    ส่งข้อมูลหน้า Dashboard:
    - stats: active/expiring_7d/trial/alerts
    - usage: [{day, usage}]
    - expiry_buckets: 0–7 / 8–30 / >30 (นับเทียบ "วันที่" และรองรับ cast)
    - expiring_soon: รายการใกล้หมด
    - system_health: ตัวอย่างสถานะระบบ
    """
    License = getattr(models, "License", None)
    Client = getattr(models, "Client", None)
    TrialRequest = getattr(models, "TrialRequest", None)
    ActivityLog = getattr(models, "ActivityLog", None)

    now = datetime.utcnow()
    today_d = today_utc_date()
    in_7 = today_d + timedelta(days=7)
    in_30 = today_d + timedelta(days=30)

    # ---------- ค่าเริ่มต้น ----------
    stats = {"active_licenses": 0, "expiring_7d": 0, "trial_licenses": 0, "alerts": "OK"}
    usage_series = []                 # [{day, usage}]
    expiry_buckets = [
        {"name": "0–7 days", "value": 0},
        {"name": "8–30 days", "value": 0},
        {"name": "> 30 days", "value": 0},
    ]
    expiring_soon = []                # [{client, days, pct}]
    system_health = [
        {"label": "API Connection", "status": "ok"},
        {"label": "Webhook Delivery", "status": "ok"},
        {"label": "Database Connection", "status": "ok"},
        {"label": "All systems operational", "status": "ok"},
    ]

    if License is None:
        return {
            "stats": stats,
            "usage": usage_series,
            "expiry_buckets": expiry_buckets,
            "expiring_soon": expiring_soon,
            "system_health": system_health,
        }

    # ---------- Stats ----------
    try:
        status_col = getattr(License, "status", None)
        expiry_raw = get_expiry_col(License)
        expiry_date_only = cast(expiry_raw, Date) if expiry_raw is not None else None
        is_trial_col = getattr(License, "is_trial", None)

        if status_col is not None:
            stats["active_licenses"] = db.query(func.count(License.id)).filter(status_col == "Active").scalar() or 0
        if expiry_date_only is not None:
            stats["expiring_7d"] = (
                db.query(func.count(License.id))
                .filter(and_(expiry_date_only.isnot(None), expiry_date_only <= in_7))
                .scalar()
                or 0
            )
        if is_trial_col is not None:
            stats["trial_licenses"] = db.query(func.count(License.id)).filter(is_trial_col == True).scalar() or 0  # noqa: E712
        elif TrialRequest is not None:
            stats["trial_licenses"] = db.query(func.count(TrialRequest.id)).scalar() or 0
    except Exception:
        pass

    # ---------- Usage (30 วันล่าสุด) ----------
    start_30 = today_d - timedelta(days=30)
    try:
        if ActivityLog is not None and hasattr(ActivityLog, "action"):
            ts_col = getattr(ActivityLog, "created_at", None) or getattr(ActivityLog, "timestamp", None)
            if ts_col is not None:
                rows = (
                    db.query(func.date(ts_col).label("d"), func.count("*"))
                    .filter(and_(ts_col >= start_30, ActivityLog.action == "activation"))
                    .group_by(func.date(ts_col))
                    .order_by(func.date(ts_col))
                    .all()
                )
                usage_series = [{"day": int(str(d)[-2:]), "usage": int(c)} for d, c in rows]
        else:
            updated_at_col = getattr(License, "updated_at", None) or getattr(License, "created_at", None)
            if updated_at_col is not None:
                rows = (
                    db.query(func.date(updated_at_col).label("d"), func.count("*"))
                    .filter(updated_at_col >= start_30)
                    .group_by(func.date(updated_at_col))
                    .order_by(func.date(updated_at_col))
                    .all()
                )
                usage_series = [{"day": int(str(d)[-2:]), "usage": int(c)} for d, c in rows]
    except Exception:
        pass

    # ---------- Expiry Buckets (robust) ----------
    try:
        if expiry_date_only is not None:
            b0 = (
                db.query(func.count(License.id))
                .filter(and_(expiry_date_only.isnot(None), expiry_date_only >= today_d, expiry_date_only <= in_7))
                .scalar()
                or 0
            )
            b1 = (
                db.query(func.count(License.id))
                .filter(and_(expiry_date_only > in_7, expiry_date_only <= in_30))
                .scalar()
                or 0
            )
            b2 = (
                db.query(func.count(License.id))
                .filter(and_(expiry_date_only > in_30))
                .scalar()
                or 0
            )
            expiry_buckets = [
                {"name": "0–7 days", "value": int(b0)},
                {"name": "8–30 days", "value": int(b1)},
                {"name": "> 30 days", "value": int(b2)},
            ]
    except Exception:
        pass

    # ---------- Expiring Soon (top 5) ----------
    try:
        if expiry_raw is not None:
            q = db.query(License).filter(getattr(License, "status", None) != "Revoked") if hasattr(License, "status") else db.query(License)
            q = q.filter(expiry_raw.isnot(None)).order_by(expiry_raw.asc()).limit(5)
            items = q.all()
            for lic in items:
                # client name (optional)
                client_name = f"Client #{getattr(lic, 'client_id', 'N/A')}"
                if Client is not None and hasattr(lic, "client_id"):
                    cli = db.query(Client).get(getattr(lic, "client_id"))
                    if cli is not None and hasattr(cli, "name") and cli.name:
                        client_name = cli.name
                # days left (ใช้วันที่)
                try:
                    exp_dt = getattr(lic, "expiry_date", None) or getattr(lic, "expiry_at", None) or getattr(lic, "expires_at", None) or getattr(lic, "expire_date", None)
                    # แปลงเป็น date ถ้าจำเป็น
                    if isinstance(exp_dt, datetime):
                        exp_d = exp_dt.date()
                    else:
                        exp_d = exp_dt
                    days_left = max((exp_d - today_d).days, 0) if exp_d else 0
                except Exception:
                    days_left = 0
                # usage % ถ้ามี
                used = getattr(lic, "activations_used", None) or getattr(lic, "devices_linked", None)
                max_act = getattr(lic, "max_activations", None)
                pct = 0
                try:
                    if isinstance(used, str) and "/" in used:
                        num, den = used.split()[0].split("/")
                        pct = int(round((int(num) / int(den)) * 100))
                    elif used is not None and max_act:
                        pct = int(round((int(used) / int(max_act)) * 100))
                except Exception:
                    pct = 0

                expiring_soon.append({"client": client_name, "days": int(days_left), "pct": max(0, min(100, pct))})
    except Exception:
        pass

    # ---------- Alerts (ตัวอย่างจาก system_health) ----------
    stats["alerts"] = "OK" if all(s["status"] == "ok" for s in system_health) else "Check"

    return {
        "stats": stats,
        "usage": usage_series,
        "expiry_buckets": expiry_buckets,
        "expiring_soon": expiring_soon,
        "system_health": system_health,
    }
