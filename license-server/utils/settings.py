# app/utils/settings.py
import os
from dataclasses import dataclass
from dotenv import load_dotenv

# โหลด .env จากรากโปรเจกต์ (ถ้าคุณวางไว้ที่อื่น ให้ปรับ path เอง)
load_dotenv()

def _b(val: str | None, default: bool = False) -> bool:
    if val is None:
        return default
    return str(val).strip().lower() in ("1", "true", "yes", "on")

@dataclass
class _Settings:
    APP_NAME: str = os.getenv("APP_NAME", "SmartAudit")
    PORTAL_URL: str = os.getenv("PORTAL_URL", "http://localhost:3000")
    SUPPORT_EMAIL: str = os.getenv("SUPPORT_EMAIL", "support@example.com")

    # ค่า SMTP จาก .env (ของคุณใช้ SMTP_TLS / SMTP_USER / MAIL_FROM)
    SMTP_HOST: str = os.getenv("SMTP_HOST", "smtp.gmail.com")
    SMTP_PORT: int = int(os.getenv("SMTP_PORT", "587"))
    SMTP_TLS: bool = _b(os.getenv("SMTP_TLS", "true"), True)
    SMTP_USER: str = os.getenv("SMTP_USER", "")
    SMTP_PASSWORD: str = os.getenv("SMTP_PASSWORD", "")
    MAIL_FROM: str = os.getenv("MAIL_FROM", "no-reply@example.com")

    # debug & เปิด/ปิดการส่งเมล
    SMTP_DEBUG: bool = _b(os.getenv("SMTP_DEBUG", "false"), False)
    EMAIL_ENABLED: bool = _b(os.getenv("EMAIL_ENABLED", "true"), True)

    # alias ให้โค้ดส่วนอื่นเรียกชื่อที่ต่างกันได้
    def __post_init__(self):
        # ให้โค้ดที่ใช้ SMTP_USE_TLS, SMTP_USERNAME, SMTP_FROM ใช้งานได้
        self.SMTP_USE_TLS = self.SMTP_TLS
        self.SMTP_USERNAME = self.SMTP_USER
        self.SMTP_FROM = self.MAIL_FROM

settings = _Settings()
