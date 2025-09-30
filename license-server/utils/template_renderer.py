import re
from datetime import datetime

_VAR_RX = re.compile(r"{{\s*([a-zA-Z0-9_\.]+)\s*}}")

def fmt_dt(dt):
    if not dt:
        return "-"
    if isinstance(dt, str):
        try:
            dt = datetime.fromisoformat(dt)
        except Exception:
            return dt
    return dt.strftime("%Y-%m-%d %H:%M")

def render_template(text: str, vars: dict) -> str:
    """แทนที่ {{var}} ด้วยค่าจาก dict (รองรับ nested a.b)"""
    def _get(path, dft="-"):
        cur = vars
        for p in path.split("."):
            if isinstance(cur, dict) and p in cur:
                cur = cur[p]
            else:
                return dft
        return cur if cur is not None else dft

    def _sub(m):
        key = m.group(1)
        val = _get(key)
        if isinstance(val, (datetime,)):
            return fmt_dt(val)
        return str(val)
    return _VAR_RX.sub(_sub, text)
