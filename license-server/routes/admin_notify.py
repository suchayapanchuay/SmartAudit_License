#from fastapi import APIRouter
#from sse_starlette.sse import EventSourceResponse
#from utils.events import subscribe, unsubscribe, sse_generator, publish
#
#router = APIRouter(prefix="/api/admin/notifications", tags=["admin-notify"])
#
#@router.get("/stream")
#async def stream():
#    q = subscribe()
#    async def _gen():
#        try:
#            async for evt in sse_generator(q):
#                yield evt
#        finally:
#            unsubscribe(q)
#    return EventSourceResponse(
#        _gen(),
#        headers={"Cache-Control": "no-cache, no-transform"},
#        ping=15,
#    )
#
#@router.post("/debug/ping")
#async def debug_ping():
#    publish("debug", {"msg": "hello from server"})
#    return {"ok": True}

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

    return EventSourceResponse(
        _gen(),
        headers={"Cache-Control": "no-cache, no-transform"},
        ping=15,
    )

@router.post("/debug/ping")
async def debug_ping():
    # ส่งทั้ง debug และ trial_request_created เพื่อให้ UI เห็นแน่นอน
    publish("debug", {"msg": "hello from server"})
    publish("trial_request_created", {
        "id": -1,
        "first_name": "System",
        "last_name": "Debug",
        "email": "debug@local",
        "company": "Server",
        "country": "TH",
        "message": "Debug ping",
    })
    return {"ok": True}


