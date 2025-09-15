# routes/admin_notify.py
from fastapi import APIRouter, Request
from starlette.responses import StreamingResponse
from utils.events import subscribe, sse_format
import asyncio, time

router = APIRouter()

@router.get("/admin/stream")
async def admin_stream(request: Request):
    async def event_generator():
        # แจ้งสถานะตอนเชื่อมสำเร็จ
        yield sse_format(None, {"type": "connected", "ts": time.time()})

        async def heartbeat():
            while True:
                await asyncio.sleep(15)
                yield ":keep-alive\n\n"  # คอมเมนต์ใน SSE

        hb = heartbeat()
        try:
            while True:
                done, _ = await asyncio.wait(
                    [asyncio.create_task(subscribe().__anext__()),
                     asyncio.create_task(hb.__anext__())],
                    return_when=asyncio.FIRST_COMPLETED
                )
                for task in done:
                    try:
                        val = task.result()
                        if isinstance(val, tuple):
                            evt_type, data = val
                            if await request.is_disconnected():
                                return
                            yield sse_format(evt_type, data)
                        else:
                            if await request.is_disconnected():
                                return
                            yield val  # keep-alive payload
                    except StopAsyncIteration:
                        return
        finally:
            try:
                await hb.aclose()
            except Exception:
                pass

    headers = {
        "Cache-Control": "no-cache",
        "Connection": "keep-alive",
        "X-Accel-Buffering": "no",
    }
    return StreamingResponse(event_generator(), media_type="text/event-stream", headers=headers)
