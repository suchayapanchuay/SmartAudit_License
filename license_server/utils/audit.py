from fastapi import Request

def get_audit_info(request: Request):
    admin = request.session.get("admin") or {}

    return {
        "actor": admin.get("email") or admin.get("username") or "unknown",
        "ip": request.client.host if request.client else None,
        "user_agent": request.headers.get("user-agent"),
    }
