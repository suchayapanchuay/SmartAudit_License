# routes/admin_notify.py
from fastapi import APIRouter
from fastapi.responses import StreamingResponse
from utils.events import sse_iter

router = APIRouter(prefix="/api/admin/notifications", tags=["AdminNotifications"])

@router.get("/stream")
async def stream_notifications():
    async def event_gen():
        async for chunk in sse_iter({"trial_request", "order_created", "order_status_changed"}):
            yield chunk
    return StreamingResponse(
        event_gen(),
        media_type="text/event-stream",
        headers={"Cache-Control": "no-cache", "Connection": "keep-alive", "X-Accel-Buffering": "no"},
    )
