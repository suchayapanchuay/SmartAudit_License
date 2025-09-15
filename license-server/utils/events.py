# utils/events.py
import asyncio
from typing import Tuple, Dict, Any

_subscribers: "set[asyncio.Queue[Tuple[str, Dict[str, Any]]]]" = set()

async def publish(event: str, data: Dict[str, Any]):
    dead = []
    for q in list(_subscribers):
        try:
            q.put_nowait((event, data))
        except Exception:
            dead.append(q)
    for q in dead:
        _subscribers.discard(q)

async def subscribe():
    """
    Async generator: yields (event_type, data)
    """
    q: asyncio.Queue = asyncio.Queue(maxsize=1000)
    _subscribers.add(q)
    try:
        while True:
            yield await q.get()
    finally:
        _subscribers.discard(q)

def sse_format(event: str | None, data: Dict[str, Any]) -> str:
    # data จะเป็น JSON stringify ที่ฝั่ง StreamingResponse จัดการ
    import json
    payload = json.dumps(data, ensure_ascii=False)
    if event:
      return f"event: {event}\n" + f"data: {payload}\n\n"
    return f"data: {payload}\n\n"
