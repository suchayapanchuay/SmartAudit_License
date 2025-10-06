# utils/mailer.py
import smtplib
from email.message import EmailMessage
from typing import Optional

# ถ้า settings อยู่ใต้ app/utils ให้ใช้บรรทัดนี้แทน:
# from app.utils.settings import settings
from utils.settings import settings

def _get(name, default=None, *alts):
    if hasattr(settings, name):
        return getattr(settings, name)
    for alt in alts:
        if hasattr(settings, alt):
            return getattr(settings, alt)
    return default

def send_email(
    to_email: str,
    subject: str,
    body: str,
    *,
    is_html: bool = False,
    text_fallback: Optional[str] = None,
) -> bool:
    # เปิดอีเมล: ถ้าไม่ตั้ง EMAIL_ENABLED ให้ถือว่า True
    email_enabled = _get("EMAIL_ENABLED", True)
    if not email_enabled:
        print(f"[EMAIL_DISABLED] to={to_email} | subject={subject}")
        return False

    host = _get("SMTP_HOST", "smtp.gmail.com")
    port = int(_get("SMTP_PORT", 587))
    use_tls = bool(_get("SMTP_USE_TLS", _get("SMTP_TLS", True)))
    username = _get("SMTP_USERNAME", _get("SMTP_USER", ""))
    password = _get("SMTP_PASSWORD", "")
    from_addr = _get("SMTP_FROM", _get("MAIL_FROM", username or "no-reply@example.com"))
    smtp_debug = bool(_get("SMTP_DEBUG", False))

    # ถ้าใช้ Gmail และ FROM ไม่ใช่บัญชีที่ล็อกอิน ให้บังคับ FROM = username (กันโดน reject)
    if "gmail.com" in host and username:
        from_addr = username

    # ต้องมี user/pass สำหรับ auth กับ Gmail/ส่วนใหญ่ของผู้ให้บริการ
    if not username or not password:
        raise RuntimeError("SMTP credentials missing: set SMTP_USER/SMTP_PASSWORD (or SMTP_USERNAME/SMTP_PASSWORD)")

    msg = EmailMessage()
    msg["From"] = from_addr
    msg["To"] = to_email
    msg["Subject"] = subject

    if is_html:
        msg.set_content(text_fallback or "Please view this email in an HTML-capable email client.")
        msg.add_alternative(body, subtype="html")
    else:
        msg.set_content(body)

    print(
        "[EMAIL_TRY] to=%s from=%s host=%s port=%s use_tls=%s is_html=%s"
        % (to_email, from_addr, host, port, use_tls, is_html)
    )

    try:
        if use_tls:
            with smtplib.SMTP(host, port, timeout=20) as s:
                if smtp_debug:
                    s.set_debuglevel(1)
                s.ehlo(); s.starttls(); s.ehlo()
                s.login(username, password)
                s.send_message(msg)
        else:
            with smtplib.SMTP_SSL(host, port, timeout=20) as s:
                if smtp_debug:
                    s.set_debuglevel(1)
                s.login(username, password)
                s.send_message(msg)

        print(f"[EMAIL_SENT] to={to_email} subject={subject}")
        return True

    except Exception as e:
        print(f"[EMAIL_ERROR] {type(e).__name__}: {e}")
        raise
