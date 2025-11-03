# routes/reports_route.py
from __future__ import annotations

from datetime import datetime, timedelta, date
from fastapi import APIRouter, Depends, Query
from sqlalchemy import and_, func, case, or_, cast, Date
from sqlalchemy.orm import Session

from database import SessionLocal
import models  # ควรมีอย่างน้อย License, Client (ActivityLog/TrialRequest จะมีหรือไม่ก็ได้)

router = APIRouter(prefix="/api/reports", tags=["reports"])

# ---------- DB Session ----------
def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()

# ---------- Helpers ----------
def parse_ymd(s: str | None) -> date | None:
    if not s:
        return None
    try:
        return datetime.strptime(s, "%Y-%m-%d").date()
    except Exception:
        return None

def get_expiry_col(License):
    return (
        getattr(License, "expiry_date", None)
        or getattr(License, "expiry_at", None)
        or getattr(License, "expires_at", None)
        or getattr(License, "expire_date", None)
    )

def build_is_trial_pred(License):
    preds = []
    if hasattr(License, "is_trial"):
        preds.append(getattr(License, "is_trial") == True)  # noqa: E712
    for colname in ("term", "type", "plan", "license_type"):
        if hasattr(License, colname):
            preds.append(func.lower(getattr(License, colname)) == "trial")
    return or_(*preds) if preds else None

# ---------- Endpoint ----------
@router.get("/metrics")
def reports_metrics(
    db: Session = Depends(get_db),
    start: str | None = Query(None, description="YYYY-MM-DD"),
    end: str | None = Query(None, description="YYYY-MM-DD"),
    range_days: int | None = Query(7, ge=1, le=365),
    license_type: str = Query("all", regex="^(all|trial|subscription|perpetual)$"),
    include_expired: bool = Query(True, description="แนบจำนวน Expired แยกต่างหาก"),
):
    """
    ข้อมูลหน้า Reports:
    - stats
    - series.purchase / series.trial
    - daily_table
    - expiry_buckets (0–7/8–30/>30) + expired_bucket (ถ้าร้องขอ)
    - top_clients
    """
    License = getattr(models, "License", None)
    Client = getattr(models, "Client", None)
    ActivityLog = getattr(models, "ActivityLog", None)
    TrialRequest = getattr(models, "TrialRequest", None)

    today_d = datetime.utcnow().date()

    s_date = parse_ymd(start)
    e_date = parse_ymd(end)
    if s_date and e_date and e_date < s_date:
        s_date, e_date = e_date, s_date
    if not s_date or not e_date:
        s_date = today_d - timedelta(days=(range_days or 7) - 1)
        e_date = today_d

    # ฟิลเตอร์ชนิด license (Optional)
    lt_filter = []
    if License is not None:
        if hasattr(License, "is_trial") and license_type == "trial":
            lt_filter.append(getattr(License, "is_trial") == True)  # noqa: E712
        elif hasattr(License, "term") and license_type in ("subscription", "perpetual"):
            lt_filter.append(getattr(License, "term") == license_type)

    def q_and(*conds):
        xs = [c for c in conds if c is not None]
        return and_(*xs) if xs else None

    # ---------- Stats ----------
    stats = {"active": 0, "expiring_7d": 0, "trial": 0, "revoked": 0}
    if License is not None:
        status_col = getattr(License, "status", None)
        expiry_raw = get_expiry_col(License)
        expiry_date_only = cast(expiry_raw, Date) if expiry_raw is not None else None
        is_trial_col = getattr(License, "is_trial", None)
        try:
            if status_col is not None:
                stats["active"] = db.query(func.count(License.id)).filter(q_and(status_col == "Active", *lt_filter)).scalar() or 0
                stats["revoked"] = db.query(func.count(License.id)).filter(q_and(status_col == "Revoked", *lt_filter)).scalar() or 0
            if expiry_date_only is not None:
                stats["expiring_7d"] = (
                    db.query(func.count(License.id))
                    .filter(q_and(expiry_date_only.isnot(None), expiry_date_only <= (today_d + timedelta(days=7)), *lt_filter))
                    .scalar()
                    or 0
                )
            if is_trial_col is not None:
                stats["trial"] = db.query(func.count(License.id)).filter(q_and(is_trial_col == True, *lt_filter)).scalar() or 0  # noqa: E712
            elif TrialRequest is not None:
                stats["trial"] = db.query(func.count(TrialRequest.id)).scalar() or 0
        except Exception:
            pass

    # ---------- Days range ----------
    days = []
    cur = s_date
    while cur <= e_date:
        days.append(cur)
        cur += timedelta(days=1)

    # ---------- Series: Purchase vs Trial ----------
    purchase_series = [0] * len(days)
    trial_series = [0] * len(days)
    is_trial_pred = build_is_trial_pred(License) if License is not None else None

    try:
        if ActivityLog is not None and hasattr(ActivityLog, "action") and License is not None:
            ts_col = getattr(ActivityLog, "created_at", None) or getattr(ActivityLog, "timestamp", None)
            lic_id_col = getattr(ActivityLog, "license_id", None)
            if ts_col is not None and lic_id_col is not None:
                if is_trial_pred is None:
                    rows = (
                        db.query(func.date(ts_col).label("d"), func.count("*").label("cnt"))
                        .join(License, License.id == lic_id_col, isouter=True)
                        .filter(and_(ts_col >= s_date, ts_col < (e_date + timedelta(days=1)), ActivityLog.action == "activation", *lt_filter))
                        .group_by(func.date(ts_col))
                        .all()
                    )
                    by_day = {r[0]: int(r[1] or 0) for r in rows}
                    for idx, d in enumerate(days):
                        purchase_series[idx] = by_day.get(d, 0)
                        trial_series[idx] = 0
                else:
                    rows = (
                        db.query(
                            func.date(ts_col).label("d"),
                            func.count("*").label("cnt"),
                            func.sum(case((is_trial_pred, 1), else_=0)).label("trial_cnt"),
                        )
                        .join(License, License.id == lic_id_col, isouter=True)
                        .filter(and_(ts_col >= s_date, ts_col < (e_date + timedelta(days=1)), ActivityLog.action == "activation", *lt_filter))
                        .group_by(func.date(ts_col))
                        .all()
                    )
                    by_day = {r[0]: r for r in rows}
                    for idx, d in enumerate(days):
                        if d in by_day:
                            _, cnt, trial_cnt = by_day[d]
                            trial_cnt = int(trial_cnt or 0)
                            purchase_cnt = int(cnt or 0) - trial_cnt
                            purchase_series[idx] = max(0, purchase_cnt)
                            trial_series[idx] = max(0, trial_cnt)
        elif License is not None:
            created_col = getattr(License, "created_at", None)
            if created_col is not None:
                if is_trial_pred is None:
                    rows = (
                        db.query(func.date(created_col).label("d"), func.count("*").label("c"))
                        .filter(and_(created_col >= s_date, created_col < (e_date + timedelta(days=1)), *lt_filter))
                        .group_by(func.date(created_col))
                        .all()
                    )
                    by_day = {r[0]: int(r[1] or 0) for r in rows}
                    for idx, d in enumerate(days):
                        purchase_series[idx] = by_day.get(d, 0)
                        trial_series[idx] = 0
                else:
                    rows = (
                        db.query(
                            func.date(created_col).label("d"),
                            func.sum(case((is_trial_pred, 0), else_=1)).label("purchase_cnt"),
                            func.sum(case((is_trial_pred, 1), else_=0)).label("trial_cnt"),
                        )
                        .filter(and_(created_col >= s_date, created_col < (e_date + timedelta(days=1)), *lt_filter))
                        .group_by(func.date(created_col))
                        .all()
                    )
                    by_day = {r[0]: r for r in rows}
                    for idx, d in enumerate(days):
                        if d in by_day:
                            _, pc, tc = by_day[d]
                            purchase_series[idx] = int(pc or 0)
                            trial_series[idx] = int(tc or 0)
    except Exception:
        pass

    # ---------- Daily table ----------
    daily_table = []
    try:
        new_by_day, expired_by_day, revoked_by_day = {}, {}, {}
        if License is not None:
            created_col = getattr(License, "created_at", None)
            expiry_raw = get_expiry_col(License)
            expiry_date_only = cast(expiry_raw, Date) if expiry_raw is not None else None
            if created_col is not None:
                rows = (
                    db.query(func.date(created_col).label("d"), func.count("*"))
                    .filter(and_(created_col >= s_date, created_col < (e_date + timedelta(days=1)), *lt_filter))
                    .group_by(func.date(created_col))
                    .all()
                )
                new_by_day = {r[0]: int(r[1] or 0) for r in rows}
            if expiry_date_only is not None:
                rows = (
                    db.query(func.date(expiry_date_only).label("d"), func.count("*"))
                    .filter(and_(expiry_date_only >= s_date, expiry_date_only < (e_date + timedelta(days=1)), *lt_filter))
                    .group_by(func.date(expiry_date_only))
                    .all()
                )
                expired_by_day = {r[0]: int(r[1] or 0) for r in rows}
        ActivityLog = getattr(models, "ActivityLog", None)
        if ActivityLog is not None and hasattr(ActivityLog, "action"):
            ts_col = getattr(ActivityLog, "created_at", None) or getattr(ActivityLog, "timestamp", None)
            if ts_col is not None:
                rows = (
                    db.query(func.date(ts_col).label("d"), func.count("*"))
                    .filter(and_(ts_col >= s_date, ts_col < (e_date + timedelta(days=1)), ActivityLog.action == "revoked"))
                    .group_by(func.date(ts_col))
                    .all()
                )
                revoked_by_day = {r[0]: int(r[1] or 0) for r in rows}
        for d in days:
            daily_table.append({
                "date": d.strftime("%d/%m/%Y"),
                "new": new_by_day.get(d, 0),
                "expired": expired_by_day.get(d, 0),
                "revoked": revoked_by_day.get(d, 0),
            })
    except Exception:
        pass

    # ---------- Expiry buckets (robust, +expired_bucket) ----------
    expiry_buckets = [
        {"name": "0–7 days", "value": 0},
        {"name": "8–30 days", "value": 0},
        {"name": "> 30 days", "value": 0},
    ]
    expired_bucket = {"name": "Expired", "value": 0}
    try:
        if License is not None:
            expiry_raw = get_expiry_col(License)
            if expiry_raw is not None:
                expiry_date_only = cast(expiry_raw, Date)
                in_7 = today_d + timedelta(days=7)
                in_30 = today_d + timedelta(days=30)
                if include_expired:
                    expired_bucket["value"] = (
                        db.query(func.count(License.id))
                        .filter(and_(expiry_date_only.isnot(None), expiry_date_only < today_d, *lt_filter))
                        .scalar()
                        or 0
                    )
                b0 = (
                    db.query(func.count(License.id))
                    .filter(and_(expiry_date_only.isnot(None), expiry_date_only >= today_d, expiry_date_only <= in_7, *lt_filter))
                    .scalar()
                    or 0
                )
                b1 = (
                    db.query(func.count(License.id))
                    .filter(and_(expiry_date_only > in_7, expiry_date_only <= in_30, *lt_filter))
                    .scalar()
                    or 0
                )
                b2 = (
                    db.query(func.count(License.id))
                    .filter(and_(expiry_date_only > in_30, *lt_filter))
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

    # ---------- Top Clients ----------
    top_clients = []
    try:
        if License is not None and Client is not None and hasattr(License, "client_id"):
            q = db.query(Client.name, func.count(License.id).label("cnt")).join(Client, Client.id == License.client_id)
            if lt_filter:
                q = q.filter(*lt_filter)
            rows = q.group_by(Client.name).order_by(func.count(License.id).desc()).limit(5).all()
            max_cnt = max((int(r[1]) for r in rows), default=1)
            for name, cnt in rows:
                pct = int(round(int(cnt) / max_cnt * 100))
                top_clients.append({"name": name or "—", "pct": pct})
    except Exception:
        pass

    resp = {
        "stats": {
            "active_licenses": stats["active"],
            "expiring_7d": stats["expiring_7d"],
            "trial_licenses": stats["trial"],
            "revoked_licenses": stats["revoked"],
        },
        "series": {
            "purchase": purchase_series,
            "trial": trial_series,
            "active": purchase_series,  # เผื่อโค้ดหน้าไหนยังอ้าง 'active'
        },
        "daily_table": daily_table,
        "expiry_buckets": expiry_buckets,
        "top_clients": top_clients,
        "range": {
            "start": s_date.strftime("%Y-%m-%d"),
            "end": e_date.strftime("%Y-%m-%d"),
            "days": len(days),
        },
    }
    if include_expired:
        resp["expired_bucket"] = expired_bucket
    return resp
