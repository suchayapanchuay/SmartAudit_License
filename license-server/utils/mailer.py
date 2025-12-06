# utils/mailer.py

"""
ตัว wrapper กลางสำหรับส่งอีเมล

จุดประสงค์:
- ให้โค้ดส่วนอื่น import แบบ: from utils.mailer import send_email
- ภายในใช้ core.email_smtp.send_email_smtp ตัวเดียว
"""

from core.email_smtp import send_email_smtp


def send_email(
    to: str,
    subject: str,
    body: str,
    is_html: bool = True,
    text_body: str | None = None,
) -> bool:
    """
    ส่งอีเมลออกไป 1 ฉบับ

    :param to: อีเมลปลายทาง
    :param subject: หัวข้ออีเมล
    :param body: เนื้อหา (html หรือ text ตาม is_html)
    :param is_html: ถ้า True จะส่งเป็น text/html
    :param text_body: เก็บ plain text เผื่อใช้ในอนาคต (ตอนนี้ยังไม่ใช้)
    :return: True ถ้าไม่ error
    """
    send_email_smtp(
        to=to,
        subject=subject,
        body=body,
        as_html=is_html,
    )
    return True


__all__ = ["send_email"]
