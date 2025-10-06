from fastapi import APIRouter, Depends, HTTPException, Query, BackgroundTasks
from pydantic import BaseModel, EmailStr
from sqlalchemy.orm import Session
from sqlalchemy import or_
from datetime import datetime, timedelta
from passlib.hash import bcrypt
from typing import List, Optional

from database import get_db
from models.client import Client
from models.client_credential import ClientCredential
from models.license import License
from models.email_templates import EmailTemplate
from utils.license_key import generate_license_key
from utils.template_renderer import render_template
from utils.mailer import send_email
from utils.settings import settings

router = APIRouter(prefix="/api", tags=["clients"])

# ---------- Schemas (input) ----------
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
    requestType: str  # "trial" | "purchase" | "support"
    search: Optional[str] = None
    source: Optional[str] = None
    sourceId: Optional[str] = None
    profile: ProfileIn
    credentials: CredIn
    trial: TrialIn

# ---------- Schemas (output) ----------
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

# ---------- CREATE ----------
@router.post("/clients", response_model=ClientOut)
def create_client(
    body: ClientCreateIn,
    background: BackgroundTasks,
    db: Session = Depends(get_db),
):
    if body.requestType not in {"trial", "purchase", "support"}:
        raise HTTPException(status_code=400, detail=f"Invalid requestType: '{body.requestType}'")

    # duplicates
    if db.query(Client).filter(Client.email == body.profile.email).first():
        raise HTTPException(status_code=409, detail="Email already exists")
    if db.query(ClientCredential).filter(ClientCredential.username == body.credentials.username).first():
        raise HTTPException(status_code=409, detail="Username already exists")

    now = datetime.utcnow()

    # create client
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
    db.flush()

    plain_pw = body.credentials.password
    cred = ClientCredential(
        client_id=client.id,
        username=body.credentials.username,
        password_hash=bcrypt.hash(plain_pw),
        created_at=now,
    )
    db.add(cred)

    # issue license
    license_key = generate_license_key()

    term: str
    product_sku: Optional[str]
    duration_days: Optional[int]
    expires_at: Optional[datetime]

    if body.requestType == "trial":
        term = "trial"
        product_sku = "SMART_AUDIT_TRIAL"
        duration_days = body.trial.days or 15
        expires_at = now + timedelta(days=duration_days)

    elif body.requestType == "purchase":
        # ✅ ให้ผู้ใช้ถือ license 1 ปี พร้อมระบุ productSku เพื่อให้ UI แสดง
        term = "purchase"                   # หมายเหตุ: ถ้า Enum เดิมไม่มีค่า 'purchase' ให้ดู models/license.py ด้านล่าง
        product_sku = "SMART_AUDIT_FULL"
        duration_days = 365
        expires_at = now + timedelta(days=365)

    else:  # support
        term = "support"
        product_sku = "SMART_AUDIT_SUPPORT"
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
    db.commit()
    db.refresh(client)
    db.refresh(lic)

    # email variables
    vars_map = {
        "client": {
            "first_name": client.first_name or "",
            "last_name": client.last_name or "",
            "email": client.email,
            "company": client.company or "",
            "country": client.country or "",
            "username": body.credentials.username,
            "plain_password": plain_pw,
        },
        "license": {
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

    tpl = (
        db.query(EmailTemplate)
        .filter(EmailTemplate.slug == "welcome", EmailTemplate.status != "Disabled")
        .first()
    )

    if tpl:
        subject = render_template(tpl.subject, vars_map)
        body_rendered = render_template(tpl.body, vars_map)
        is_html = bool(tpl.is_html)
    else:
        subject = f"Your {settings.APP_NAME} Account & License"
        body_rendered = f"""
Hello {vars_map['client']['first_name']} {vars_map['client']['last_name']},

Account
- Email: {vars_map['client']['email']}
- Username: {vars_map['client']['username']}
- Password: {vars_map['client']['plain_password']}

License
- Key: {vars_map['license']['license_key']}
- Type: {vars_map['license']['term']}
- SKU: {vars_map['license']['product_sku']}
- Expires: {vars_map['license']['expires_at'] or '-'}

Login: {vars_map['meta']['portal_url']}
""".strip()
        is_html = False

    if is_html:
        background.add_task(
            send_email,
            client.email,
            subject,
            body_rendered,
            is_html=True,
            text_fallback="Please open with an HTML-capable email client."
        )
    else:
        html_pre = f"<pre style='font-family:ui-monospace,Menlo,Consolas,monospace'>{body_rendered}</pre>"
        background.add_task(
            send_email,
            client.email,
            subject,
            html_pre,
            is_html=True,
            text_fallback=body_rendered
        )

    return _to_out(client, lic)

# ---------- LIST ----------
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
    type: Optional[str] = Query(None, regex="^(trial|purchase|support)$"),
    limit: int = Query(200, ge=1, le=500),
    offset: int = Query(0, ge=0),
):
    qry = db.query(Client)
    if q:
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

# ---------- GET BY ID ----------
@router.get("/clients/{client_id}", response_model=ClientOut)
def get_client(client_id: int, db: Session = Depends(get_db)):
    c = db.query(Client).filter(Client.id == client_id).first()
    if not c:
        raise HTTPException(status_code=404, detail="Client not found")

    lic = db.query(License).filter(License.client_id == client_id).order_by(License.id.desc()).first()
    return _to_out(c, lic)

# ---------- LICENSES BY CLIENT ----------
@router.get("/clients/{client_id}/licenses")
def get_client_licenses(client_id: int, db: Session = Depends(get_db)):
    rows = db.query(License).filter(License.client_id == client_id).order_by(License.id.desc()).all()
    return [
        {
            "id": lic.id,
            "licenseKey": lic.license_key,
            "type": lic.term,                                       # "trial" | "purchase" | "support"
            "productSku": lic.product_sku or "-",                   # ▶ ไม่ให้ว่าง
            "issuedAt": lic.issued_at.isoformat() if lic.issued_at else None,
            "expiresAt": lic.expires_at.isoformat() if lic.expires_at else None,
            "status": lic.status,
        }
        for lic in rows
    ]

# ---------- DELETE ----------
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

# ===== UPDATE =====
from typing import Any, Dict

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
            cred.password_hash = bcrypt.hash(body.credentials.password)

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
    lic = db.query(License).filter(License.client_id == client_id).order_by(License.id.desc()).first()
    return _to_out(c, lic)
