# # utils/template_renderer.py
# from typing import Dict, Any
# from jinja2 import Environment, BaseLoader, select_autoescape

# # Jinja2 environment สำหรับใช้ {{ ... }}
# _env = Environment(
#     loader=BaseLoader(),
#     autoescape=select_autoescape(["html", "xml"]),
# )

# def render_template(template_str: str, variables: Dict[str, Any]) -> str:
#     """
#     render string ที่มี {{ ... }} ด้วย Jinja2
#     รองรับ nested dict เช่น {{ client.username }}, {{ license.license_key }}
#     """
#     if not template_str:
#         return ""
#     try:
#         tmpl = _env.from_string(template_str)
#         # สำคัญ: ใช้ **variables → ทำให้ client / license / meta เป็นตัวแปรใน template
#         return tmpl.render(**variables)
#     except Exception as e:
#         print("[TEMPLATE ERROR]", e)
#         # ถ้า template syntax พัง ให้คืนข้อความเดิม กันแอปพัง
#         return template_str

# utils/template_renderer.py
from jinja2 import Environment, StrictUndefined, TemplateError

# สร้าง Jinja2 environment
env = Environment(
    autoescape=False,          # ให้เป็น plain text/HTML ตามที่ส่งเข้า
    undefined=StrictUndefined  # ถ้าใช้ตัวแปรที่ไม่มี จะ error ชัด ๆ
)

def render_template(template_text: str, variables: dict) -> str:
    """
    Render ข้อความด้วย Jinja2 template
    สามารถใช้ตัวแปร nested เช่น {{ client.first_name }}, {{ license.license_key }}, {{ meta.app_name }}
    """
    if not template_text:
        return ""
    try:
        template = env.from_string(template_text)
        return template.render(**variables)
    except TemplateError as e:
        # ถ้า template มีปัญหา ให้คืนข้อความ error ติดมา (จะช่วย debug ได้)
        return f"[TEMPLATE ERROR] {type(e).__name__}: {e}"
