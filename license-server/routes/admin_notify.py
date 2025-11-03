# routes/admin_notify.py
from fastapi import APIRouter
from sse_starlette.sse import EventSourceResponse
from utils.events import subscribe, unsubscribe, sse_generator, publish

router = APIRouter(prefix="/api/admin/notifications", tags=["admin-notify"])

@router.get("/stream")
async def stream():
    q = subscribe()

    async def _gen():
        try:
            async for evt in sse_generator(q):
                yield evt
        finally:
            unsubscribe(q)

    # sse_starlette จะส่ง keepalive/ping ให้อยู่แล้วถ้าระบุ ping=
    return EventSourceResponse(
        _gen(),
        headers={"Cache-Control": "no-cache, no-transform"},
        ping=15,
    )

@router.post("/debug/ping")
async def debug_ping():
    publish("debug", {"msg": "hello from server"})
    # ตัวอย่างยิงได้ทั้งสองชนิด
    publish("trial_request", {
        "id": -1, "firstName": "System", "lastName": "Debug",
        "email": "debug@local", "company": "Server", "country": "TH",
        "message": "Debug trial"
    })
    publish("order_created", {
        "id": -2, "order_code": "ORD-DEBUG",
        "customer_email": "debug@local", "customer_name": "System Debug",
        "company": "Server", "phone": "-", "items": [],
        "grand_total": 0, "currency": "THB", "status": "pending",
        "note": "Debug order", "created_at": None
    })
    return {"ok": True}
