# utils/template_renderer.py
import re
from datetime import datetime

_VAR_RX = re.compile(r"{{\s*([a-zA-Z0-9_\.]+)(?:\|([a-zA-Z0-9_]+)(?::([^}]+))?)?\s*}}")

def fmt_dt(dt, pattern="%Y-%m-%d %H:%M"):
    if not dt:
        return "-"
    if isinstance(dt, str):
        try:
            dt = datetime.fromisoformat(dt)
        except Exception:
            return dt
    return dt.strftime(pattern)

def render_template(text: str, vars: dict) -> str:
    """แทนที่ {{var|filter:arg}}; รองรับ nested a.b; filters: default, fmt"""

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
        flt = m.group(2)
        arg = m.group(3)
        val = _get(key)

        if flt == "default":
            dft = arg if arg is not None else "-"
            val = val if val not in (None, "-", "") else dft

        if flt == "fmt":
            pattern = arg if arg else "%Y-%m-%d %H:%M"
            try:
                return fmt_dt(val, pattern)
            except Exception:
                return str(val)

        if isinstance(val, datetime):
            return fmt_dt(val)
        return str(val)

    return _VAR_RX.sub(_sub, text)
