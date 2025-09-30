# utils/mailer.py
import smtplib
from email.message import EmailMessage
from typing import Optional
from utils.settings import settings

def send_email(
    to_email: str,
    subject: str,
    body: str,
    *,
    is_html: bool = False,
    text_fallback: Optional[str] = None,
):
    """
    ส่งอีเมลแบบ text หรือ HTML (ถ้า is_html=True และมี text_fallback -> ส่งเป็น multipart/alternative)
    """
    if not settings.EMAIL_ENABLED:
        print(f"[EMAIL_DISABLED] to={to_email} | subject={subject}")
        print(body)
        return

    msg = EmailMessage()
    msg["From"] = settings.SMTP_FROM
    msg["To"] = to_email
    msg["Subject"] = subject

    if is_html:
        msg.set_content(text_fallback or "Please view this email in an HTML-capable client.")
        msg.add_alternative(body, subtype="html")
    else:
        msg.set_content(body)

    if settings.SMTP_USE_TLS:
        with smtplib.SMTP(settings.SMTP_HOST, settings.SMTP_PORT) as s:
            s.starttls()
            s.login(settings.SMTP_USERNAME, settings.SMTP_PASSWORD)
            s.send_message(msg)
    else:
        with smtplib.SMTP_SSL(settings.SMTP_HOST, settings.SMTP_PORT) as s:
            s.login(settings.SMTP_USERNAME, settings.SMTP_PASSWORD)
            s.send_message(msg)
