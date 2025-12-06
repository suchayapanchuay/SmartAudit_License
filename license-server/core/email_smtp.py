# # core/email_smtp.py
# import os
# import smtplib
# import ssl
# from email.mime.text import MIMEText
# from email.utils import formataddr


# def _get_bool_env(name: str, default: bool = False) -> bool:
#     val = os.getenv(name)
#     if val is None:
#         return default
#     return str(val).strip().lower() in ("1", "true", "yes", "y", "on")


# SMTP_HOST = os.getenv("SMTP_HOST", "localhost")
# SMTP_PORT = int(os.getenv("SMTP_PORT", "587"))
# SMTP_USER = os.getenv("SMTP_USER", "")
# SMTP_PASS = os.getenv("SMTP_PASS", "")

# SMTP_FROM_NAME = os.getenv("SMTP_FROM_NAME", "SmartAudit")
# SMTP_FROM_EMAIL = os.getenv("SMTP_FROM_EMAIL", SMTP_USER or "no-reply@example.com")

# SMTP_USE_TLS = _get_bool_env("SMTP_USE_TLS", True)
# SMTP_USE_SSL = _get_bool_env("SMTP_USE_SSL", False)
# SMTP_TIMEOUT = int(os.getenv("SMTP_TIMEOUT", "20"))
# SMTP_VERIFY_SSL = _get_bool_env("SMTP_VERIFY_SSL", True)  


# def _build_ssl_context() -> ssl.SSLContext:
#     """
#     สร้าง SSL context ตามค่า SMTP_VERIFY_SSL
#     """
#     if SMTP_VERIFY_SSL:
#         return ssl.create_default_context()
#     else:

#         ctx = ssl._create_unverified_context()
#         ctx.check_hostname = False
#         ctx.verify_mode = ssl.CERT_NONE
#         return ctx


# def send_email_smtp(
#     to: str,
#     subject: str,
#     body: str,
#     as_html: bool = True,
# ) -> None:
#     if not SMTP_HOST:
#         raise RuntimeError("SMTP_HOST is not configured")

#     subtype = "html" if as_html else "plain"
#     msg = MIMEText(body, subtype, "utf-8")
#     msg["Subject"] = subject
#     msg["From"] = formataddr((SMTP_FROM_NAME, SMTP_FROM_EMAIL))
#     msg["To"] = to

#     try:
#         if SMTP_USE_SSL:
#             context = _build_ssl_context()
#             with smtplib.SMTP_SSL(
#                 SMTP_HOST,
#                 SMTP_PORT,
#                 timeout=SMTP_TIMEOUT,
#                 context=context,
#             ) as smtp:
#                 if SMTP_USER:
#                     smtp.login(SMTP_USER, SMTP_PASS)
#                 smtp.send_message(msg)

#         else:
#             with smtplib.SMTP(
#                 SMTP_HOST,
#                 SMTP_PORT,
#                 timeout=SMTP_TIMEOUT,
#             ) as smtp:
#                 if SMTP_USE_TLS:
#                     context = _build_ssl_context()
#                     smtp.starttls(context=context)

#                 if SMTP_USER:
#                     smtp.login(SMTP_USER, SMTP_PASS)

#                 smtp.send_message(msg)

#     except Exception as e:
#         raise RuntimeError(f"SMTP error: {e}") from e

# core/email_smtp.py
import smtplib
import ssl
from email.mime.text import MIMEText
from email.utils import formataddr

from utils.settings import settings


def _build_ssl_context() -> ssl.SSLContext:
    if settings.SMTP_VERIFY_SSL:
        return ssl.create_default_context()
    else:
        ctx = ssl._create_unverified_context()
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        return ctx


def _maybe_login(smtp: smtplib.SMTP) -> None:
    """
    login เฉพาะกรณี:
      - มี SMTP_USERNAME
      - server รองรับ AUTH จริง ๆ
    """
    if not settings.SMTP_USERNAME:
        if settings.SMTP_DEBUG:
            print("[SMTP] no username configured, skip login()")
        return

    features = getattr(smtp, "esmtp_features", {}) or {}
    if "auth" not in features:
        if settings.SMTP_DEBUG:
            print("[SMTP] server has no AUTH extension, skip login()")
        return

    if settings.SMTP_DEBUG:
        print("[SMTP] login() with user:", settings.SMTP_USERNAME)

    smtp.login(settings.SMTP_USERNAME, settings.SMTP_PASS)


def send_email_smtp(
    to: str,
    subject: str,
    body: str,
    as_html: bool = True,
) -> None:
    if not settings.EMAIL_ENABLED:
        if settings.SMTP_DEBUG:
            print("[SMTP] EMAIL_ENABLED = false, skip sending to:", to)
        return

    if not settings.SMTP_HOST:
        raise RuntimeError("SMTP_HOST is not configured")

    subtype = "html" if as_html else "plain"
    msg = MIMEText(body, subtype, "utf-8")
    msg["Subject"] = subject
    msg["From"] = formataddr((settings.SMTP_FROM_NAME, settings.SMTP_FROM_EMAIL))
    msg["To"] = to

    if settings.SMTP_DEBUG:
        print(
            "[SMTP CONFIG]",
            "host=", settings.SMTP_HOST,
            "port=", settings.SMTP_PORT,
            "user=", settings.SMTP_USERNAME or "(no-auth)",
            "use_tls=", settings.SMTP_USE_TLS,
            "use_ssl=", settings.SMTP_USE_SSL,
            "timeout=", settings.SMTP_TIMEOUT,
        )

    try:
        # ---------- โหมด SSL (เช่น port 465) ----------
        if settings.SMTP_USE_SSL:
            context = _build_ssl_context()
            if settings.SMTP_DEBUG:
                print("[SMTP] connecting via SMTP_SSL ...")

            with smtplib.SMTP_SSL(
                settings.SMTP_HOST,
                settings.SMTP_PORT,
                timeout=settings.SMTP_TIMEOUT,
                context=context,
            ) as smtp:
                smtp.set_debuglevel(1 if settings.SMTP_DEBUG else 0)
                smtp.ehlo()

                # login ถ้า server รองรับ AUTH
                _maybe_login(smtp)

                if settings.SMTP_DEBUG:
                    print("[SMTP] send_message() ...")
                smtp.send_message(msg)

        # ---------- โหมด plain / STARTTLS ----------
        else:
            if settings.SMTP_DEBUG:
                print("[SMTP] connecting via SMTP (plain) ...")

            with smtplib.SMTP(
                settings.SMTP_HOST,
                settings.SMTP_PORT,
                timeout=settings.SMTP_TIMEOUT,
            ) as smtp:
                smtp.set_debuglevel(1 if settings.SMTP_DEBUG else 0)

                # EHLO รอบแรก
                smtp.ehlo()

                # STARTTLS ถ้าเปิดไว้
                if settings.SMTP_USE_TLS:
                    if settings.SMTP_DEBUG:
                        print("[SMTP] starttls() ...")
                    context = _build_ssl_context()
                    smtp.starttls(context=context)
                    smtp.ehlo()  # EHLO อีกรอบหลัง TLS

                # login ถ้า server รองรับ AUTH
                _maybe_login(smtp)

                if settings.SMTP_DEBUG:
                    print("[SMTP] send_message() ...")
                smtp.send_message(msg)

    except Exception as e:
        raise RuntimeError(f"SMTP error: {e}") from e
