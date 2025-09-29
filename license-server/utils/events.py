#import asyncio
#import json
#from typing import AsyncGenerator, Dict, Any, List
#
#_subscribers: List[asyncio.Queue] = []
#
#def _event_wrap(event_type: str, data: Dict[str, Any]) -> str:
#    return json.dumps({"type": event_type, "data": data}, ensure_ascii=False)
#
#async def _broadcast(msg: str) -> None:
#    for q in list(_subscribers):
#        try:
#            q.put_nowait(msg)
#        except Exception:
#            pass
#
#def publish(event_type: str, data: Dict[str, Any]) -> None:
#    # เรียกได้จาก async context (endpoint async)
#    msg = _event_wrap(event_type, data)
#    asyncio.create_task(_broadcast(msg))
#
#def subscribe() -> asyncio.Queue:
#    q: asyncio.Queue = asyncio.Queue()
#    _subscribers.append(q)
#    return q
#
#def unsubscribe(q: asyncio.Queue) -> None:
#    try:
#        _subscribers.remove(q)
#    except ValueError:
#        pass
#
#async def sse_generator(q: asyncio.Queue) -> AsyncGenerator[str, None]:
#    try:
#        ping_task = asyncio.create_task(_ping(q))
#        while True:
#            data = await q.get()
#            yield f"data: {data}\n\n"
#    except asyncio.CancelledError:
#        pass
#    finally:
#        ping_task.cancel()
#
#async def _ping(q: asyncio.Queue):
#    while True:
#        await asyncio.sleep(25)
#        try:
#            q.put_nowait(_event_wrap("ping", {"ts": asyncio.get_event_loop().time()}))
#        except Exception:
#            pass

import asyncio
import json
from typing import AsyncGenerator, Dict, Any, List, Optional

_subscribers: List[asyncio.Queue] = []
_main_loop: Optional[asyncio.AbstractEventLoop] = None

def set_main_loop(loop: asyncio.AbstractEventLoop) -> None:
    global _main_loop
    _main_loop = loop

def _event_wrap(event_type: str, data: Dict[str, Any]) -> str:
    return json.dumps({"type": event_type, "data": data}, ensure_ascii=False)

async def _broadcast(msg: str) -> None:
    for q in list(_subscribers):
        try:
            q.put_nowait(msg)
        except Exception:
            pass

def publish(event_type: str, data: Dict[str, Any]) -> None:
    msg = _event_wrap(event_type, data)
    try:
        loop = asyncio.get_running_loop()
        loop.create_task(_broadcast(msg))
    except RuntimeError:
        if _main_loop and _main_loop.is_running():
            asyncio.run_coroutine_threadsafe(_broadcast(msg), _main_loop)
        else:
            for q in list(_subscribers):
                try:
                    q.put_nowait(msg)
                except Exception:
                    pass

def subscribe() -> asyncio.Queue:
    q: asyncio.Queue = asyncio.Queue()
    _subscribers.append(q)
    return q

def unsubscribe(q: asyncio.Queue) -> None:
    try:
        _subscribers.remove(q)
    except ValueError:
        pass

async def sse_generator(q: asyncio.Queue) -> AsyncGenerator[str, None]:
    try:
        ping_task = asyncio.create_task(_ping(q))
        while True:
            data = await q.get()
            yield f"data: {data}\n\n"
    except asyncio.CancelledError:
        pass
    finally:
        ping_task.cancel()

async def _ping(q: asyncio.Queue):
    while True:
        await asyncio.sleep(25)
        try:
            q.put_nowait(_event_wrap("ping", {"ts": asyncio.get_event_loop().time()}))
        except Exception:
            pass
