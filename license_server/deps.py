from fastapi import Header, HTTPException, Request, Depends
from starlette import status as st
from sqlalchemy.orm import Session
from datetime import datetime, timezone
from .database import get_session
from .models import ApiKey, ApiKeyUsage, KeyStatus
from .security.api_keys import sha256_hex

# (ตัวอย่าง) ใช้กับ endpoint ฝั่ง service-to-service ถ้าต้องการ
async def require_api_key(
    request: Request,
    x_api_key: str = Header(None),
    db: Session = Depends(get_session),
):
    if not x_api_key:
        raise HTTPException(st.HTTP_401_UNAUTHORIZED, "Missing X-API-Key")

    last4 = x_api_key[-4:]
    token_parts = x_api_key.split("_")
    key_prefix = "_".join(token_parts[:2]) if len(token_parts) >= 2 else ""

    candidates = (
        db.query(ApiKey)
        .filter(ApiKey.key_prefix == key_prefix, ApiKey.key_last4 == last4)
        .all()
    )
    key_obj = None
    token_hash = sha256_hex(x_api_key)
    for cand in candidates:
        if cand.key_hash == token_hash:
            key_obj = cand
            break

    if not key_obj:
        raise HTTPException(st.HTTP_401_UNAUTHORIZED, "Invalid API Key")

    if key_obj.status != KeyStatus.active:
        raise HTTPException(st.HTTP_403_FORBIDDEN, "API Key not active")

    if key_obj.expires_at and key_obj.expires_at.replace(tzinfo=timezone.utc) < datetime.now(timezone.utc):
        raise HTTPException(st.HTTP_403_FORBIDDEN, "API Key expired")

    # log (อย่างง่าย)
    key_obj.last_used_at = datetime.utcnow()
    db.add(
        ApiKeyUsage(
            api_key_id=key_obj.id,
            method=request.method,
            path=str(request.url.path),
            status_code=200,
            ip_addr=request.client.host if request.client else None,
        )
    )
    db.commit()
    return key_obj

# (ตัวอย่าง) ตรวจ scope
def require_scope(required: str):
    def checker(key_obj: ApiKey = Depends(require_api_key)):
        scopes = (key_obj.scopes_json or {}).get("scopes", [])
        if "full_access" in scopes or required in scopes:
            return key_obj
        raise HTTPException(status_code=403, detail="Insufficient scope")
    return checker
