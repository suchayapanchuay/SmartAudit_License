# routes/credentials.py
from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel
from sqlalchemy.orm import Session
from passlib.hash import bcrypt
import secrets, string
from datetime import datetime

from database import get_db
from models.client import Client
from models.client_credential import ClientCredential
from utils.mailer import send_email

router = APIRouter(prefix="/api/clients", tags=["client-credentials"])

# ---------- helpers ----------
def _gen_password(length: int = 12) -> str:
    # ตัวอย่าง policy แบบง่าย (ปรับตามต้องการ)
    alphabet = string.ascii_letters + string.digits
    return "".join(secrets.choice(alphabet) for _ in range(length))

# ---------- schemas ----------
class CredOut(BaseModel):
    client_id: int
    username: str
    created_at: datetime

class ResetIn(BaseModel):
    length: int = 12                 # ความยาวรหัสชั่วคราว (ปรับได้)
    send_email: bool = True          # ส่งอีเมลแจ้งลูกค้าหรือไม่
    email_to: str | None = None      # ถ้าไม่ระบุ จะใช้ client.email
    notify_subject: str | None = None
    notify_body_text: str | None = None   # body แบบ text (ปลอดภัยสุด)
    notify_body_html: str | None = None   # ถ้าจะส่งแบบ HTML

class ResetOut(BaseModel):
    client_id: int
    username: str
    temporary_password: str

# ---------- GET username (ไม่มี password) ----------
@router.get("/{client_id}/credentials", response_model=CredOut)
def get_credential(client_id: int, db: Session = Depends(get_db)):
    cred = db.query(ClientCredential).filter(ClientCredential.client_id == client_id).first()
    if not cred:
        raise HTTPException(status_code=404, detail="Credential not found")
    return CredOut(client_id=cred.client_id, username=cred.username, created_at=cred.created_at)

# ---------- RESET password (return plaintext once) ----------
@router.post("/{client_id}/credentials/reset", response_model=ResetOut)
def reset_password(client_id: int, body: ResetIn, db: Session = Depends(get_db)):
    client = db.query(Client).filter(Client.id == client_id).first()
    if not client:
        raise HTTPException(status_code=404, detail="Client not found")

    cred = db.query(ClientCredential).filter(ClientCredential.client_id == client_id).first()
    if not cred:
        raise HTTPException(status_code=404, detail="Credential not found")

    # generate & hash
    length = max(8, min(64, body.length or 12))
    temp_pwd = _gen_password(length)
    cred.password_hash = bcrypt.hash(temp_pwd)
    db.commit()

    # ส่งอีเมล (optional)
    if body.send_email:
        to_email = body.email_to or getattr(client, "email", None)
        if to_email:
            subject = body.notify_subject or "Your SmartAudit account password has been reset"
            # สร้างเนื้อหาอีเมลเริ่มต้น ถ้าไม่ส่งมาเอง
            default_text = (
                f"Hello {getattr(client, 'first_name', '') or ''},\n\n"
                f"Your SmartAudit password has been reset.\n\n"
                f"Username: {cred.username}\n"
                f"Temporary password: {temp_pwd}\n\n"
                f"Please log in and change your password immediately.\n"
            )

            if body.notify_body_html:
                # ให้ priority กับ HTML ถ้าส่งมา พร้อม text fallback
                send_email(
                    to_email,
                    subject,
                    body.notify_body_html,
                    is_html=True,
                    text_fallback=body.notify_body_text or default_text,
                )
            else:
                # ส่ง text ธรรมดา
                send_email(
                    to_email,
                    subject,
                    body.notify_body_text or default_text,
                    is_html=False,
                )

    return ResetOut(client_id=client_id, username=cred.username, temporary_password=temp_pwd)
