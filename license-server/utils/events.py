# utils/events.py
import asyncio
import json
from typing import Any, Dict, Optional

class EventBus:
    def __init__(self):
        self._subs: list[asyncio.Queue] = []
        self._lock = asyncio.Lock()

    async def subscribe(self) -> asyncio.Queue:
        q: asyncio.Queue = asyncio.Queue(maxsize=1000)
        async with self._lock:
            self._subs.append(q)
        return q

    async def unsubscribe(self, q: asyncio.Queue):
        async with self._lock:
            if q in self._subs:
                self._subs.remove(q)

    async def publish(self, event_name: str, data: Dict[str, Any]):
        payload = {"event": event_name, "data": data}
        async with self._lock:
            for q in list(self._subs):
                try:
                    q.put_nowait(payload)
                except asyncio.QueueFull:
                    try: self._subs.remove(q)
                    except ValueError: pass

_bus = EventBus()

async def publish(event_name: str, data: Dict[str, Any]):
    await _bus.publish(event_name, data)

async def sse_iter(event_filter: Optional[set[str]] = None):
    q = await _bus.subscribe()
    try:
        while True:
            item = await q.get()
            name = item.get("event")
            data = item.get("data")
            if event_filter and name not in event_filter:
                continue
            yield f"event: {name}\n" + "data: " + json.dumps(data, ensure_ascii=False) + "\n\n"
    finally:
        await _bus.unsubscribe(q)
