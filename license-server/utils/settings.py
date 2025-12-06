# # app/utils/settings.py
# import os
# from dataclasses import dataclass
# from dotenv import load_dotenv

# # โหลด .env จากรากโปรเจกต์ (ถ้าคุณวางไว้ที่อื่น ให้ปรับ path เอง)
# load_dotenv()

# def _b(val: str | None, default: bool = False) -> bool:
#     if val is None:
#         return default
#     return str(val).strip().lower() in ("1", "true", "yes", "on")

# @dataclass
# class _Settings:
#     APP_NAME: str = os.getenv("APP_NAME", "SmartAudit")
#     PORTAL_URL: str = os.getenv("PORTAL_URL", "http://localhost:3000")
#     SUPPORT_EMAIL: str = os.getenv("SUPPORT_EMAIL", "support@example.com")

#     # ค่า SMTP จาก .env (ของคุณใช้ SMTP_TLS / SMTP_USER / MAIL_FROM)
#     SMTP_HOST: str = os.getenv("SMTP_HOST", "smtp.gmail.com")
#     SMTP_PORT: int = int(os.getenv("SMTP_PORT", "587"))
#     SMTP_TLS: bool = _b(os.getenv("SMTP_TLS", "true"), True)
#     SMTP_USER: str = os.getenv("SMTP_USER", "")
#     SMTP_PASSWORD: str = os.getenv("SMTP_PASSWORD", "")
#     MAIL_FROM: str = os.getenv("MAIL_FROM", "no-reply@example.com")

#     # debug & เปิด/ปิดการส่งเมล
#     SMTP_DEBUG: bool = _b(os.getenv("SMTP_DEBUG", "false"), False)
#     EMAIL_ENABLED: bool = _b(os.getenv("EMAIL_ENABLED", "true"), True)

#     # alias ให้โค้ดส่วนอื่นเรียกชื่อที่ต่างกันได้
#     def __post_init__(self):
#         # ให้โค้ดที่ใช้ SMTP_USE_TLS, SMTP_USERNAME, SMTP_FROM ใช้งานได้
#         self.SMTP_USE_TLS = self.SMTP_TLS
#         self.SMTP_USERNAME = self.SMTP_USER
#         self.SMTP_FROM = self.MAIL_FROM

# settings = _Settings()

# utils/settings.py
import os
from dataclasses import dataclass
from dotenv import load_dotenv

# โหลด .env จากรากโปรเจกต์
load_dotenv()


def _b(val: str | None, default: bool = False) -> bool:
    """
    แปลง string เป็น bool:
    true/1/yes/on → True (ไม่สนตัวพิมพ์เล็กใหญ่)
    """
    if val is None:
        return default
    return str(val).strip().lower() in ("1", "true", "yes", "on")


@dataclass
class _Settings:
    # =====================
    # แอปหลัก
    # =====================
    APP_NAME: str = os.getenv("APP_NAME", "SmartAudit")
    PORTAL_URL: str = os.getenv("PORTAL_URL", "http://localhost:3000")
    SUPPORT_EMAIL: str = os.getenv("SUPPORT_EMAIL", "support@example.com")

    # =====================
    # SMTP / EMAIL CONFIG
    # =====================
    SMTP_HOST: str = os.getenv("SMTP_HOST", "smtp.gmail.com")
    SMTP_PORT: int = int(os.getenv("SMTP_PORT", "587"))

    _tls_env: str | None = os.getenv("SMTP_TLS") or os.getenv("SMTP_USE_TLS")
    SMTP_TLS: bool = _b(_tls_env, True)

    _ssl_env: str | None = os.getenv("SMTP_SSL") or os.getenv("SMTP_USE_SSL")
    SMTP_SSL: bool = _b(_ssl_env, False)

    SMTP_USER: str = os.getenv("SMTP_USER", "")
    SMTP_PASSWORD: str = os.getenv("SMTP_PASSWORD") or os.getenv("SMTP_PASS", "")

    _mail_from_env: str | None = os.getenv("MAIL_FROM") or os.getenv("SMTP_FROM_EMAIL")
    MAIL_FROM: str = _mail_from_env or "no-reply@example.com"

    SMTP_FROM_NAME: str = os.getenv("SMTP_FROM_NAME", "SmartAudit")

    SMTP_TIMEOUT: int = int(os.getenv("SMTP_TIMEOUT", "20"))
    SMTP_VERIFY_SSL: bool = _b(os.getenv("SMTP_VERIFY_SSL", "true"), True)

    SMTP_DEBUG: bool = _b(os.getenv("SMTP_DEBUG", "false"), False)
    EMAIL_ENABLED: bool = _b(os.getenv("EMAIL_ENABLED", "true"), True)

    # =====================
    # alias / compatibility
    # =====================
    def __post_init__(self):
        self.SMTP_USE_TLS = self.SMTP_TLS
        self.SMTP_USE_SSL = self.SMTP_SSL

        self.SMTP_USERNAME = self.SMTP_USER
        self.SMTP_PASS = self.SMTP_PASSWORD

        self.SMTP_FROM = self.MAIL_FROM or self.SMTP_USER or "no-reply@example.com"
        self.SMTP_FROM_EMAIL = self.SMTP_FROM

        if not self.SMTP_FROM_NAME:
            self.SMTP_FROM_NAME = self.APP_NAME


settings = _Settings()
