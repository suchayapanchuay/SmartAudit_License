# routes/admin_api_keys.py
from fastapi import APIRouter, Depends, HTTPException, Response, status, Query
from sqlalchemy.orm import Session
from sqlalchemy import or_
from datetime import datetime, timedelta, timezone

from database import get_db
from models.api_key import ApiKey, KeyStatus
from schemas.api import ApiKeyCreateIn, ApiKeyCreatedOut, ApiKeyListItem
from security.api_keys import generate_plain_key, sha256_hex, mask_from
from services.activities import log_activity  # <= helper บันทึก log

router = APIRouter(prefix="/api/admin/api-keys", tags=["admin-api-keys"])


def admin_required():
    # TODO: ใช้ auth จริง (session/JWT)
    return True


# ---------- Create ----------
@router.post("", response_model=ApiKeyCreatedOut)
def create_api_key(
    payload: ApiKeyCreateIn,
    db: Session = Depends(get_db),
    _: bool = Depends(admin_required),
):
    plain = generate_plain_key()  # เช่น sk_live_xxx_xxx
    parts = plain.split("_")
    prefix = "_".join(parts[:2]) if len(parts) >= 2 else plain[:8]
    last4 = plain[-4:]

    expires_at = None
    if payload.expires_in_days:
        # เก็บเป็น naive UTC ให้สอดคล้องกับ model ที่ใช้อยู่
        expires_utc = datetime.now(timezone.utc) + timedelta(days=payload.expires_in_days)
        expires_at = expires_utc.replace(tzinfo=None)

    row = ApiKey(
        name=payload.name.strip(),
        key_prefix=prefix,
        key_last4=last4,
        key_hash=sha256_hex(plain),
        scopes_json={"scopes": payload.scopes or []},
        status=payload.status,
        expires_at=expires_at,
        created_by="admin@example.com",  # TODO: ดึงจาก session จริง
        created_at=datetime.utcnow(),
    )

    # ใส่ทั้ง resource และ activity ในหนึ่งทรานแซกชัน
    db.add(row)
    db.flush()  # ให้ได้ r.id ก่อนสำหรับ log

    # บันทึก activity
    log_activity(
        db,
        actor="admin@example.com",
        action="api_key.created",
        target_type="api_key",
        target_id=row.id,
        message=f"Created API key '{row.name}'",
        meta={
            "mask": mask_from(prefix, last4),
            "scopes": row.scopes_json.get("scopes", []),
            "status": row.status,
            "expires_at": expires_at.isoformat() if expires_at else None,
        },
    )

    db.commit()
    db.refresh(row)

    return ApiKeyCreatedOut(
        id=row.id,
        name=row.name,
        scopes=row.scopes_json.get("scopes", []),
        status=row.status,
        expires_at=row.expires_at,
        plaintext_key=plain,  # แสดงครั้งเดียว!
        mask=mask_from(prefix, last4),
    )


# ---------- List ----------
@router.get("", response_model=list[ApiKeyListItem])
def list_api_keys(
    db: Session = Depends(get_db),
    _: bool = Depends(admin_required),
    q: str | None = Query(default=None, description="ค้นหาชื่อ/แมสก์/สโคป"),
    status_: KeyStatus | None = Query(default=None, alias="status"),
    scope: str | None = Query(default=None, description="ต้องมีสโคปนี้"),
    page: int = Query(default=1, ge=1),
    page_size: int = Query(default=50, ge=1, le=200),
):
    query = db.query(ApiKey)

    if q:
        like = f"%{q}%"
        query = query.filter(
            or_(
                ApiKey.name.ilike(like),
                ApiKey.key_prefix.ilike(like),
                ApiKey.key_last4.ilike(like),
            )
        )

    if status_:
        query = query.filter(ApiKey.status == status_)

    query = query.order_by(ApiKey.created_at.desc())

    offset = (page - 1) * page_size
    rows = query.offset(offset).limit(page_size).all()

    # กรอง scope ใน Python (รองรับทุกเวอร์ชันของ MariaDB/MySQL)
    if scope:
        rows = [r for r in rows if scope in (r.scopes_json or {}).get("scopes", [])]

    return [
        ApiKeyListItem(
            id=r.id,
            name=r.name,
            key_mask=mask_from(r.key_prefix, r.key_last4),
            scopes=(r.scopes_json or {}).get("scopes", []),
            status=r.status,
            lastUsed=r.last_used_at.isoformat() if getattr(r, "last_used_at", None) else None,
            expiresAt=r.expires_at.isoformat() if r.expires_at else None,
        )
        for r in rows
    ]


# ---------- Revoke (เปลี่ยนสถานะ) ----------
@router.post("/{key_id}/revoke", status_code=status.HTTP_204_NO_CONTENT)
def revoke_api_key(
    key_id: int,
    db: Session = Depends(get_db),
    _: bool = Depends(admin_required),
):
    r = db.get(ApiKey, key_id)
    if not r:
        raise HTTPException(status_code=404, detail="API key not found")

    r.status = KeyStatus.revoked
    r.revoked_at = datetime.utcnow()

    # บันทึก activity ก่อน commit
    log_activity(
        db,
        actor="admin@example.com",
        action="api_key.revoked",
        target_type="api_key",
        target_id=r.id,
        message=f"Revoked API key '{r.name}'",
        meta={"mask": mask_from(r.key_prefix, r.key_last4)},
    )

    db.add(r)
    db.commit()
    return Response(status_code=status.HTTP_204_NO_CONTENT)


# ---------- Delete (ลบทิ้งจริง) ----------
@router.delete("/{key_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_api_key(
    key_id: int,
    db: Session = Depends(get_db),
    _: bool = Depends(admin_required),
):
    r = db.get(ApiKey, key_id)
    if not r:
        raise HTTPException(status_code=404, detail="API key not found")

    # log ก่อนลบ (จะได้มีข้อมูล mask/ชื่อใน log)
    log_activity(
        db,
        actor="admin@example.com",
        action="api_key.deleted",
        target_type="api_key",
        target_id=r.id,
        message=f"Deleted API key '{r.name}'",
        meta={"mask": mask_from(r.key_prefix, r.key_last4)},
    )

    db.delete(r)
    db.commit()
    return Response(status_code=status.HTTP_204_NO_CONTENT)
