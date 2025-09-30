# utils/settings.py (เวอร์ชันไม่พึ่ง pydantic)
import os

class _Settings:
    # SMTP
    SMTP_HOST = os.getenv("SMTP_HOST", "smtp.gmail.com")
    SMTP_PORT = int(os.getenv("SMTP_PORT", "587"))
    SMTP_USERNAME = os.getenv("SMTP_USERNAME", "no-reply@yourdomain.com")
    SMTP_PASSWORD = os.getenv("SMTP_PASSWORD", "app-password-or-secret")
    SMTP_FROM = os.getenv("SMTP_FROM", "SmartAudit <no-reply@yourdomain.com>")
    SMTP_USE_TLS = os.getenv("SMTP_USE_TLS", "true").lower() in ("1", "true", "yes")
    EMAIL_ENABLED = os.getenv("EMAIL_ENABLED", "true").lower() in ("1", "true", "yes")

    # App meta
    APP_NAME = os.getenv("APP_NAME", "SmartAudit")
    PORTAL_URL = os.getenv("PORTAL_URL", "http://localhost:3000")

settings = _Settings()
