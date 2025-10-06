from fastapi import APIRouter, Depends, HTTPException, Query
from pydantic import BaseModel
from typing import List, Optional
from sqlalchemy.orm import Session

from database import get_db
from models.email_templates import EmailTemplate
from utils.template_renderer import render_template

router = APIRouter(prefix="/api/email-templates", tags=["email-templates"])

# -------- Schemas --------
class EmailTemplateIn(BaseModel):
    slug: str
    name: str
    subject: str
    body: str
    status: str = "Active"
    is_html: bool = False

class EmailTemplatePatch(BaseModel):
    name: Optional[str] = None
    subject: Optional[str] = None
    body: Optional[str] = None
    status: Optional[str] = None
    is_html: Optional[bool] = None

# --- replace this whole class (Pydantic v2) ---
from datetime import datetime
from pydantic import BaseModel
from pydantic import ConfigDict  # v2

class EmailTemplateOut(BaseModel):
    id: int
    slug: str
    name: str
    subject: str
    body: str
    status: str
    is_html: bool
    created_at: datetime | None = None
    updated_at: datetime | None = None

    # v2 style (แทน orm_mode)
    model_config = ConfigDict(from_attributes=True)
# --- end replace ---


# -------- CRUD --------
@router.get("", response_model=List[EmailTemplateOut])
def list_templates(db: Session = Depends(get_db), q: Optional[str] = Query(None)):
    qry = db.query(EmailTemplate)
    if q:
        like = f"%{q}%"
        qry = qry.filter((EmailTemplate.name.ilike(like)) | (EmailTemplate.subject.ilike(like)) | (EmailTemplate.slug.ilike(like)))
    return qry.order_by(EmailTemplate.updated_at.desc()).all()

@router.get("/{template_id}", response_model=EmailTemplateOut)
def get_template(template_id: int, db: Session = Depends(get_db)):
    t = db.query(EmailTemplate).filter(EmailTemplate.id == template_id).first()
    if not t:
        raise HTTPException(status_code=404, detail="Template not found")
    return t

@router.post("", response_model=EmailTemplateOut)
def create_template(body: EmailTemplateIn, db: Session = Depends(get_db)):
    if db.query(EmailTemplate).filter(EmailTemplate.slug == body.slug).first():
        raise HTTPException(status_code=409, detail="Slug already exists")
    t = EmailTemplate(**body.dict())
    db.add(t)
    db.commit()
    db.refresh(t)
    return t

@router.patch("/{template_id}", response_model=EmailTemplateOut)
def update_template(template_id: int, body: EmailTemplatePatch, db: Session = Depends(get_db)):
    t = db.query(EmailTemplate).filter(EmailTemplate.id == template_id).first()
    if not t:
        raise HTTPException(status_code=404, detail="Template not found")
    for k, v in body.dict(exclude_unset=True).items():
        setattr(t, k, v)
    db.commit()
    db.refresh(t)
    return t

@router.delete("/{template_id}")
def delete_template(template_id: int, db: Session = Depends(get_db)):
    t = db.query(EmailTemplate).filter(EmailTemplate.id == template_id).first()
    if not t:
        raise HTTPException(status_code=404, detail="Template not found")
    db.delete(t)
    db.commit()
    return {"ok": True}

# -------- Helpers --------
@router.get("/variables/list")
def list_variables():
    # ตัวแปรมาตรฐานที่รองรับตอนส่ง Welcome/License
    return {
        "client": ["first_name", "last_name", "email", "company", "country", "username", "plain_password"],
        "license": ["license_key", "term", "product_sku", "expires_at", "issued_at", "max_activations", "activations_used", "status"],
        "meta": ["app_name", "portal_url"],
        "examples": [
            "Hello {{client.first_name}}",
            "Your license key is {{license.license_key}}",
            "Expires at {{license.expires_at}}",
        ]
    }

class RenderPreviewIn(BaseModel):
    subject: str
    body: str
    variables: dict

@router.post("/render/preview")
def render_preview(body: RenderPreviewIn):
    return {
        "subject": render_template(body.subject, body.variables),
        "body": render_template(body.body, body.variables),
    }