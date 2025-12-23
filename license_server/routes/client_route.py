# client_route.py
from datetime import datetime, timedelta
from typing import List, Optional, Literal

from fastapi import APIRouter, Depends, HTTPException, Query, BackgroundTasks
from pydantic import BaseModel, EmailStr
from sqlalchemy import or_
from sqlalchemy.orm import Session

from license_server.database  import get_db
from license_server.models.client import Client
from license_server.models.client_credential import ClientCredential
from license_server.models.license import License
from license_server.models.email_templates import EmailTemplate
from license_server.utils.license_key import generate_license_key
from license_server.utils.template_renderer import render_template
from license_server.utils.mailer import send_email
from license_server.utils.settings import settings
from license_server.utils.passwords import hash_password  

router = APIRouter(prefix="/api", tags=["clients"])

# =========================
#          SCHEMAS
# =========================

# ---------- input ----------
class ProfileIn(BaseModel):
    firstName: str
    lastName: str
    email: EmailStr
    phone: Optional[str] = None
    company: Optional[str] = None
    industry: Optional[str] = None
    country: Optional[str] = None
    message: Optional[str] = None
    estimateUser: Optional[int] = None

class CredIn(BaseModel):
    username: str
    password: str

class TrialIn(BaseModel):
    days: Optional[int] = None

class ClientCreateIn(BaseModel):
    requestType: Literal["trial", "purchase", "support"]
    search: Optional[str] = None
    source: Optional[str] = None
    sourceId: Optional[str] = None
    profile: ProfileIn
    credentials: CredIn
    trial: TrialIn

# ---------- output ----------
class ClientOut(BaseModel):
    id: int
    requestType: str
    firstName: str
    lastName: str
    email: EmailStr

    phone: Optional[str] = None
    company: Optional[str] = None
    industry: Optional[str] = None
    country: Optional[str] = None
    message: Optional[str] = None
    estimateUser: Optional[int] = None

    createdAt: Optional[str] = None
    licenseKey: Optional[str] = None
    licenseExpiresAt: Optional[str] = None


def _to_out(c: Client, lic: Optional[License] = None) -> ClientOut:
    return ClientOut(
        id=c.id,
        requestType=c.request_type,
        firstName=c.first_name or "",
        lastName=c.last_name or "",
        email=c.email,
        phone=c.phone,
        company=c.company,
        industry=c.industry,
        country=c.country,
        message=c.message,
        estimateUser=c.estimate_user,
        createdAt=c.created_at.isoformat() if c.created_at else None,
        licenseKey=(lic.license_key if lic else None),
        licenseExpiresAt=(lic.expires_at.isoformat() if (lic and lic.expires_at) else None),
    )

# =========================
#          CREATE
# =========================

@router.post("/clients", response_model=ClientOut)
def create_client(
    body: ClientCreateIn,
    background: BackgroundTasks,
    db: Session = Depends(get_db),
):
    # --- ตรวจซ้ำ email / username ---
    if db.query(Client).filter(Client.email == body.profile.email).first():
        raise HTTPException(status_code=409, detail="Email already exists")
    if db.query(ClientCredential).filter(ClientCredential.username == body.credentials.username).first():
        raise HTTPException(status_code=409, detail="Username already exists")

    now = datetime.utcnow()

    # --- สร้าง client ---
    client = Client(
        request_type=body.requestType,
        source=body.source,
        source_id=body.sourceId,
        first_name=body.profile.firstName,
        last_name=body.profile.lastName,
        email=body.profile.email,
        phone=body.profile.phone,
        company=body.profile.company,
        industry=body.profile.industry,
        country=body.profile.country,
        message=body.profile.message,
        estimate_user=body.profile.estimateUser,
        trial_days=body.trial.days,
        created_at=now,
    )
    db.add(client)
    db.flush()  # ให้ได้ client.id มาก่อน

    # --- CREDENTIALS (จำ plain password ไว้ใช้ในอีเมล) ---
    plain_pw = body.credentials.password  # <= รหัสจริงที่ user กรอก
    cred = ClientCredential(
        client_id=client.id,
        username=body.credentials.username,
        password_hash=hash_password(plain_pw),  # เก็บเฉพาะ hash ใน DB
        created_at=now,
    )
    db.add(cred)

    # --- LICENSE ---
    license_key = generate_license_key()

    req = body.requestType  # 'trial' | 'purchase' | 'support'
    term = req
    product_sku: Optional[str] = None
    duration_days: Optional[int] = None
    expires_at: Optional[datetime] = None

    if req == "trial":
        product_sku = "SMART_AUDIT_TRIAL"
        duration_days = body.trial.days or 15
        expires_at = now + timedelta(days=duration_days)
    elif req == "purchase":
        product_sku = "SMART_AUDIT_FULL"
        duration_days = 365
        expires_at = now + timedelta(days=365)
    else:  # support
        product_sku = None
        duration_days = None
        expires_at = None

    lic = License(
        client_id=client.id,
        license_key=license_key,
        term=term,
        product_sku=product_sku,
        duration_days=duration_days,
        max_activations=1,
        activations_used=0,
        status="active",
        issued_at=now,
        expires_at=expires_at,
        created_at=now,
    )
    db.add(lic)

    # --- COMMIT ---
    try:
        db.commit()
    except Exception as e:
        db.rollback()
        raise HTTPException(status_code=500, detail=f"DB Commit failed: {type(e).__name__}: {e}")

    db.refresh(client)
    db.refresh(lic)

    # ดึง credential ล่าสุด เผื่ออนาคตมีการเปลี่ยน username
    cred = (
        db.query(ClientCredential)
        .filter(ClientCredential.client_id == client.id)
        .first()
    )

    # --- EMAIL VARIABLES (สำคัญ: plain_password อยู่ตรงนี้) ---
    vars_map = {
        "client": {
            "id": client.id,
            "request_type": client.request_type,
            "first_name": client.first_name or "",
            "last_name": client.last_name or "",
            "email": client.email,
            "company": client.company or "",
            "country": client.country or "",
            "username": (cred.username if cred else body.credentials.username),
            # ✅ ส่งรหัสจริงเข้า template
            "plain_password": plain_pw,
        },
        "license": {
            "id": lic.id,
            "license_key": lic.license_key,
            "term": lic.term,
            "product_sku": lic.product_sku or "-",
            "expires_at": lic.expires_at.isoformat() if lic.expires_at else None,
            "issued_at": lic.issued_at.isoformat() if lic.issued_at else None,
            "max_activations": lic.max_activations,
            "activations_used": lic.activations_used,
            "status": lic.status,
        },
        "meta": {
            "app_name": settings.APP_NAME,
            "portal_url": settings.PORTAL_URL,
        }
    }

    print("EMAIL VARS_MAP =", vars_map)  # debug ดูใน log ได้ว่ารหัสมาจริง

    # --- RENDER TEMPLATE ---
    tpl = None
    try:
        tpl = (
            db.query(EmailTemplate)
            .filter(EmailTemplate.slug == "welcome", EmailTemplate.status != "Disabled")
            .first()
        )
    except Exception:
        tpl = None

    if tpl:
        subject = render_template(tpl.subject, vars_map)
        body_rendered = render_template(tpl.body, vars_map)
        is_html = bool(tpl.is_html)
    else:
        # fallback ถ้าไม่มี template welcome ให้ใช้ข้อความ default
        subject = f"Your {settings.APP_NAME} Account & License"
        body_rendered = f"""
Hello {vars_map['client']['first_name']} {vars_map['client']['last_name']},

Account details
- Email: {vars_map['client']['email']}
- Username: {vars_map['client']['username']}
- Temporary password: {vars_map['client']['plain_password']}

License information
- License key: {vars_map['license']['license_key']}
- Product: {vars_map['license']['product_sku']}
- License type: {vars_map['license']['term']}
- Expiry date: {vars_map['license']['expires_at'] or '-'}

To get started, please follow these steps:
1) Go to: {vars_map['meta']['portal_url']}
2) Log in with your username and temporary password above
3) (Recommended) Change your password after your first login
4) Activate your license using the license key shown above
""".strip()
        is_html = False

    # --- SEND EMAIL ---
    if is_html:
        # ถ้า template เป็น HTML
        background.add_task(
            send_email,
            client.email,
            subject,
            body_rendered,
            True,
            "Please open with an HTML-capable email client.",
        )
    else:
        html_pre = (
            "<pre style='font-family:ui-monospace,Menlo,Consolas,monospace'>"
            f"{body_rendered}</pre>"
        )
        background.add_task(
            send_email,
            client.email,
            subject,
            html_pre,
            True,
            body_rendered,
        )

    return _to_out(client, lic)



# =========================
#           LIST
# =========================

class ClientListItem(BaseModel):
    id: int
    firstName: str
    lastName: str
    email: EmailStr
    company: Optional[str] = None
    requestType: str
    createdAt: Optional[str] = None

@router.get("/clients", response_model=List[ClientListItem])
def list_clients(
    db: Session = Depends(get_db),
    q: Optional[str] = Query(None),
    email: Optional[EmailStr] = Query(None),   # exact email filter
    type: Optional[Literal["trial","purchase","support"]] = Query(None),
    limit: int = Query(200, ge=1, le=500),
    offset: int = Query(0, ge=0),
):
    qry = db.query(Client)

    if email:
        qry = qry.filter(Client.email == str(email))
    elif q:
        kw = f"%{q.strip()}%"
        qry = qry.filter(or_(
            Client.first_name.ilike(kw),
            Client.last_name.ilike(kw),
            Client.email.ilike(kw),
            Client.company.ilike(kw),
        ))

    if type:
        qry = qry.filter(Client.request_type == type)

    rows = qry.order_by(Client.id.desc()).offset(offset).limit(limit).all()
    return [
        ClientListItem(
            id=c.id,
            firstName=c.first_name or "",
            lastName=c.last_name or "",
            email=c.email,
            company=c.company,
            requestType=c.request_type,
            createdAt=c.created_at.isoformat() if c.created_at else None,
        )
        for c in rows
    ]

# =========================
#          GET BY ID
# =========================

@router.get("/clients/{client_id}", response_model=ClientOut)
def get_client(client_id: int, db: Session = Depends(get_db)):
    c = db.query(Client).filter(Client.id == client_id).first()
    if not c:
        raise HTTPException(status_code=404, detail="Client not found")

    lic = (
        db.query(License)
        .filter(License.client_id == client_id)
        .order_by(License.id.desc())
        .first()
    )
    return _to_out(c, lic)

# =========================
#     LICENSES BY CLIENT
# =========================

@router.get("/clients/{client_id}/licenses")
def get_client_licenses(client_id: int, db: Session = Depends(get_db)):
    exists = db.query(Client.id).filter(Client.id == client_id).first()
    if not exists:
        raise HTTPException(status_code=404, detail="Client not found")

    rows = (
        db.query(License)
        .filter(License.client_id == client_id)
        .order_by(License.id.desc())
        .all()
    )
    return [
        {
            "id": lic.id,
            "licenseKey": lic.license_key,
            "type": lic.term,
            "productSku": lic.product_sku or "-",
            "issuedAt": lic.issued_at.isoformat() if lic.issued_at else None,
            "expiresAt": lic.expires_at.isoformat() if lic.expires_at else None,
            "status": lic.status,
        }
        for lic in rows
    ]

# =========================
#          DELETE
# =========================

@router.delete("/clients/{client_id}")
def delete_client(client_id: int, db: Session = Depends(get_db)):
    client = db.query(Client).filter(Client.id == client_id).first()
    if not client:
        raise HTTPException(status_code=404, detail="Client not found")

    db.query(ClientCredential).filter(ClientCredential.client_id == client_id).delete()
    db.query(License).filter(License.client_id == client_id).delete()

    db.delete(client)
    db.commit()
    return {"ok": True, "message": f"Client {client_id} deleted"}

# =========================
#           UPDATE
# =========================

class ProfilePatch(BaseModel):
    firstName: Optional[str] = None
    lastName: Optional[str] = None
    email: Optional[EmailStr] = None
    phone: Optional[str] = None
    company: Optional[str] = None
    industry: Optional[str] = None
    country: Optional[str] = None
    message: Optional[str] = None
    estimateUser: Optional[int] = None

class CredPatch(BaseModel):
    username: Optional[str] = None
    password: Optional[str] = None

class TrialPatch(BaseModel):
    days: Optional[int] = None

class ClientPatchIn(BaseModel):
    requestType: Optional[str] = None           # "trial" | "purchase" | "support"
    profile: Optional[ProfilePatch] = None
    credentials: Optional[CredPatch] = None
    trial: Optional[TrialPatch] = None

@router.patch("/clients/{client_id}", response_model=ClientOut)
def update_client(client_id: int, body: ClientPatchIn, db: Session = Depends(get_db)):
    c = db.query(Client).filter(Client.id == client_id).first()
    if not c:
        raise HTTPException(status_code=404, detail="Client not found")

    # update basic fields
    if body.requestType in {"trial", "purchase", "support"}:
        c.request_type = body.requestType

    if body.profile:
        p = body.profile
        if p.firstName is not None: c.first_name = p.firstName
        if p.lastName  is not None: c.last_name  = p.lastName
        if p.email     is not None: c.email      = p.email
        if p.phone     is not None: c.phone      = p.phone
        if p.company   is not None: c.company    = p.company
        if p.industry  is not None: c.industry   = p.industry
        if p.country   is not None: c.country    = p.country
        if p.message   is not None: c.message    = p.message
        if p.estimateUser is not None: c.estimate_user = p.estimateUser

    # update credential (optional)
    if body.credentials:
        cred = db.query(ClientCredential).filter(ClientCredential.client_id == client_id).first()
        if not cred:
            cred = ClientCredential(client_id=client_id, username="", password_hash="")
            db.add(cred)
        if body.credentials.username is not None:
            cred.username = body.credentials.username
        if body.credentials.password:
            cred.password_hash = hash_password(body.credentials.password)  # ✅

    # update license if requestType == trial and trial.days provided
    if body.trial and body.trial.days is not None:
        lic = (
            db.query(License)
            .filter(License.client_id == client_id)
            .order_by(License.id.desc())
            .first()
        )
        if lic and c.request_type == "trial":
            lic.duration_days = body.trial.days
            lic.expires_at = (lic.issued_at or datetime.utcnow()) + timedelta(days=body.trial.days)

    db.commit()

    # latest license for response
    lic = (
        db.query(License)
        .filter(License.client_id == client_id)
        .order_by(License.id.desc())
        .first()
    )
    return _to_out(c, lic)

# =========================
#       CREDENTIAL APIs
# =========================

def _gen_password(length: int = 12) -> str:
    import secrets, string
    alphabet = string.ascii_letters + string.digits
    return "".join(secrets.choice(alphabet) for _ in range(length))

def _unique_username(db: Session, base: str) -> str:
    base = (base or "user").strip()[:100] or "user"
    username, i = base, 1
    while db.query(ClientCredential).filter(ClientCredential.username == username).first():
        i += 1
        username = f"{base}{i}"
    return username

class CredOut(BaseModel):
    client_id: int
    username: str
    created_at: Optional[datetime] = None

class ResetIn(BaseModel):
    length: int = 12
    send_email: bool = True
    email_to: Optional[EmailStr] = None
    notify_subject: Optional[str] = None
    notify_body_text: Optional[str] = None
    notify_body_html: Optional[str] = None

class ResetOut(BaseModel):
    client_id: int
    username: str
    temporary_password: str

@router.get("/clients/{client_id}/credentials", response_model=CredOut)
def get_credential(client_id: int, db: Session = Depends(get_db)):
    cred = db.query(ClientCredential).filter(ClientCredential.client_id == client_id).first()
    if not cred:
        raise HTTPException(status_code=404, detail="Credential not found")
    return CredOut(client_id=cred.client_id, username=cred.username, created_at=cred.created_at)

@router.post("/clients/{client_id}/credentials/reset", response_model=ResetOut)
def reset_password(
    client_id: int,
    body: ResetIn,
    background: BackgroundTasks,
    db: Session = Depends(get_db),
):
    client = db.query(Client).filter(Client.id == client_id).first()
    if not client:
        raise HTTPException(status_code=404, detail="Client not found")

    cred = db.query(ClientCredential).filter(ClientCredential.client_id == client_id).first()
    if not cred:
        base_username = (client.email.split("@")[0] if client.email else f"User{client_id}")
        cred = ClientCredential(
            client_id=client_id,
            username=_unique_username(db, base_username),
            password_hash=hash_password(_gen_password(10)),  # ✅
            created_at=datetime.utcnow(),
        )
        db.add(cred)
        db.flush()

    # สร้าง temp password ใหม่
    length = max(8, min(64, body.length or 12))
    temp_pwd = _gen_password(length)
    cred.password_hash = hash_password(temp_pwd)  # ✅
    db.commit()

    # ส่งอีเมลแจ้ง (ถ้าเลือก)
    if body.send_email:
        to_email = body.email_to or client.email
        if to_email:
            subject = body.notify_subject or f"Your {settings.APP_NAME} account password has been reset"
            default_text = (
                f"Hello {client.first_name or ''},\n\n"
                f"A temporary password has been generated for your account.\n\n"
                f"Username: {cred.username}\n"
                f"Temporary password: {temp_pwd}\n\n"
                f"Login: {settings.PORTAL_URL}\n"
                f"Please log in and change your password immediately.\n"
            )
            if body.notify_body_html:
                html = body.notify_body_html.replace("{{username}}", cred.username).replace("{{password}}", temp_pwd)
                text = (body.notify_body_text or default_text).replace("{{username}}", cred.username).replace("{{password}}", temp_pwd)
                background.add_task(send_email, to_email, subject, html, True, text)
            else:
                text = (body.notify_body_text or default_text).replace("{{username}}", cred.username).replace("{{password}}", temp_pwd)
                background.add_task(send_email, to_email, subject, text, False, None)

    return ResetOut(client_id=client_id, username=cred.username, temporary_password=temp_pwd)

# =========================
#        DEBUG EMAIL
# =========================

class TestEmailIn(BaseModel):
    to: EmailStr
    subject: str = "SMTP direct test"
    body: str = "<h3>Hello</h3> SMTP test"
    is_html: bool = True

@router.get("/_debug/email/config")
def debug_email_config():
    # อย่าระบุ password ออกมา
    return {
        "EMAIL_ENABLED": getattr(settings, "EMAIL_ENABLED", None),
        "SMTP_HOST": getattr(settings, "SMTP_HOST", None),
        "SMTP_PORT": getattr(settings, "SMTP_PORT", None),
        "SMTP_USE_TLS": getattr(settings, "SMTP_USE_TLS", None),
        "SMTP_USERNAME": getattr(settings, "SMTP_USERNAME", None),
        "SMTP_FROM": getattr(settings, "SMTP_FROM", None),
        "SMTP_DEBUG": getattr(settings, "SMTP_DEBUG", None),
    }

@router.post("/_debug/email/send")
def debug_email_send(body: TestEmailIn):
    try:
        ok = send_email(body.to, body.subject, body.body, is_html=body.is_html)
        return {"ok": ok}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"{type(e).__name__}: {e}")
