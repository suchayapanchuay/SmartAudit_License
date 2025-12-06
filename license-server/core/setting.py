# # core/setting.py
# from pydantic_settings import BaseSettings, SettingsConfigDict
# from pydantic import EmailStr


# class Settings(BaseSettings):

#     SMTP_HOST: str = "192.168.121.39"
#     SMTP_PORT: int = 587
#     SMTP_USER: EmailStr = "sales@smartclick.co.th"
#     SMTP_PASS: str = ""
#     SMTP_FROM_NAME: str = "SmartAudit"
#     SMTP_FROM_EMAIL: EmailStr | None = None

#     model_config = SettingsConfigDict(
#         env_file=".env",
#         extra="ignore",
#     )


# settings = Settings()

# if settings.SMTP_FROM_EMAIL is None:
#     settings.SMTP_FROM_EMAIL = settings.SMTP_USER
