# # backend/services/email_templates.py
# from typing import Any, Dict

# from sqlalchemy.orm import Session
# from jinja2 import Environment, BaseLoader, select_autoescape

# from models.email_templates import EmailTemplate

# # Jinja2 environment สำหรับ render {{ ... }}
# env = Environment(
#     loader=BaseLoader(),
#     autoescape=select_autoescape(["html", "xml"]),
# )


# def render_text(template_str: str, variables: Dict[str, Any]) -> str:
#     """
#     render string ที่มี {{ variable }} ด้วย Jinja2
#     """
#     if not template_str:
#         return ""
#     try:
#         template = env.from_string(template_str)
#         return template.render(**variables)
#     except Exception:
#         # ถ้า template พัง (syntax error) ให้ส่งข้อความเดิมกลับไป
#         return template_str


# def render_subject_and_body(
#     subject: str,
#     body: str,
#     variables: Dict[str, Any],
# ) -> dict:
#     """
#     ใช้สำหรับ endpoint preview (ไม่เกี่ยวกับ DB)
#     """
#     rendered_subject = render_text(subject, variables)
#     rendered_body = render_text(body, variables)
#     return {"subject": rendered_subject, "body": rendered_body}


# def render_template_by_id(
#     db: Session,
#     template_id: str,
#     variables: Dict[str, Any],
# ) -> dict:
#     """
#     ดึง EmailTemplate จาก DB แล้ว render subject + body
#     ใช้กับ:
#     - send-test
#     - ส่งอีเมลจริงตอน issue license ฯลฯ
#     """
#     tpl: EmailTemplate | None = (
#         db.query(EmailTemplate).filter(EmailTemplate.id == template_id).first()
#     )
#     if not tpl:
#         raise ValueError("Template not found")

#     rendered = render_subject_and_body(
#         subject=tpl.subject or "",
#         body=tpl.body or "",
#         variables=variables,
#     )
#     rendered["is_html"] = bool(tpl.is_html)
#     return rendered

# services/email_templates.py
from typing import Any, Dict
from sqlalchemy.orm import Session

from jinja2 import Environment, BaseLoader, select_autoescape

from models.email_templates import EmailTemplate

# Jinja2 environment สำหรับ render {{ ... }}
env = Environment(
    loader=BaseLoader(),
    autoescape=select_autoescape(["html", "xml"]),
)


def render_text(template_str: str, variables: Dict[str, Any]) -> str:
    """
    render string ที่มี {{ variable }} ด้วย Jinja2
    """
    if not template_str:
        return ""
    try:
        template = env.from_string(template_str)
        return template.render(**variables)
    except Exception:
        # ถ้า template พัง (syntax error) ให้ส่งข้อความเดิมกลับไป
        return template_str


def render_subject_and_body(
    subject: str,
    body: str,
    variables: Dict[str, Any],
) -> dict:
    """
    ใช้สำหรับ endpoint preview (ไม่เกี่ยวกับ DB)
    """
    rendered_subject = render_text(subject, variables)
    rendered_body = render_text(body, variables)
    return {"subject": rendered_subject, "body": rendered_body}


def render_template_by_id(
    db: Session,
    template_id: str,
    variables: Dict[str, Any],
) -> dict:
    """
    ดึง EmailTemplate จาก DB แล้ว render subject + body
    ใช้กับ:
    - send-test
    - ส่งอีเมลจริงตอน issue license ฯลฯ
    """
    tpl: EmailTemplate | None = (
        db.query(EmailTemplate).filter(EmailTemplate.id == template_id).first()
    )
    if not tpl:
        raise ValueError("Template not found")

    rendered = render_subject_and_body(
        subject=tpl.subject or "",
        body=tpl.body or "",
        variables=variables,
    )
    rendered["is_html"] = bool(tpl.is_html)
    return rendered
