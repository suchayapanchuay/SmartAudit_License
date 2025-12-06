# # app/routes/email_template_route.py
# from typing import Any, Dict, List, Optional
# from datetime import datetime
# from uuid import uuid4

# from fastapi import APIRouter, Depends, HTTPException, status
# from pydantic import BaseModel, EmailStr, ConfigDict
# from sqlalchemy.orm import Session
# from sqlalchemy.exc import IntegrityError, DataError

# from database import get_db
# from core.email_smtp import send_email_smtp
# from services.email_templates import (
#     render_subject_and_body,
#     render_template_by_id,
# )
# from models.email_templates import EmailTemplate

# router = APIRouter(prefix="/api/email-templates", tags=["email-templates"])


# # ========= SCHEMAS =========

# class PreviewRequest(BaseModel):
#     subject: str
#     body: str
#     variables: Dict[str, Any] | None = None


# class PreviewResponse(BaseModel):
#     subject: str
#     body: str


# class SendTestRequest(BaseModel):
#     to: EmailStr
#     variables: Dict[str, Any] | None = None


# class EmailTemplateBase(BaseModel):
#     slug: str
#     name: str
#     subject: str
#     body: str
#     status: str = "Active"
#     is_html: bool = True


# class EmailTemplateCreate(EmailTemplateBase):
#     pass


# class EmailTemplateUpdate(BaseModel):
#     name: Optional[str] = None
#     subject: Optional[str] = None
#     body: Optional[str] = None
#     status: Optional[str] = None
#     is_html: Optional[bool] = None


# class EmailTemplateRead(EmailTemplateBase):
#     id: str
#     updated_at: datetime | None = None

#     model_config = ConfigDict(from_attributes=True)


# # ========= ENDPOINTS =========

# @router.get("/variables/list")
# def list_variables():
#     return {
#         "client": [
#             "first_name",
#             "last_name",
#             "email",
#             "username",
#             "plain_password",
#         ],
#         "license": [
#             "license_key",
#             "term",
#             "product_sku",
#             "expires_at",
#         ],
#         "meta": [
#             "app_name",
#             "portal_url",
#         ],
#     }


# @router.get("", response_model=List[EmailTemplateRead])
# def list_email_templates(db: Session = Depends(get_db)):
#     templates = (
#         db.query(EmailTemplate)
#         .order_by(EmailTemplate.updated_at.desc())
#         .all()
#     )
#     return templates


# @router.post("", response_model=EmailTemplateRead)
# def create_template(payload: EmailTemplateCreate, db: Session = Depends(get_db)):

#     for key in ["slug", "name", "subject", "body"]:
#         if not getattr(payload, key, None):
#             raise HTTPException(status_code=400, detail=f"Missing field: {key}")

#     existed = (
#         db.query(EmailTemplate)
#         .filter(EmailTemplate.slug == payload.slug)
#         .first()
#     )
#     if existed:
#         raise HTTPException(400, detail="Slug already exists")

#     tpl = EmailTemplate(
#         id=str(uuid4()),               # UUID ตรงกับ CHAR(36)
#         slug=payload.slug,
#         name=payload.name,
#         subject=payload.subject,
#         body=payload.body,
#         status=payload.status,
#         is_html=payload.is_html,
#     )

#     try:
#         db.add(tpl)
#         db.commit()
#         db.refresh(tpl)
#     except IntegrityError as e:
#         db.rollback()
#         raise HTTPException(400, detail=f"Database integrity error: {e.orig}")
#     except DataError as e:
#         db.rollback()
#         raise HTTPException(400, detail=f"Database data error: {e.orig}")

#     return tpl


# @router.get("/{template_id}", response_model=EmailTemplateRead)
# def get_email_template(template_id: str, db: Session = Depends(get_db)):
#     tpl = db.query(EmailTemplate).filter(EmailTemplate.id == template_id).first()
#     if not tpl:
#         raise HTTPException(404, "Template not found")
#     return tpl


# @router.patch("/{template_id}", response_model=EmailTemplateRead)
# def update_email_template(
#     template_id: str,
#     payload: EmailTemplateUpdate,
#     db: Session = Depends(get_db),
# ):
#     tpl = db.query(EmailTemplate).filter(EmailTemplate.id == template_id).first()
#     if not tpl:
#         raise HTTPException(404, "Template not found")

#     data = payload.model_dump(exclude_unset=True)
#     for field, value in data.items():
#         setattr(tpl, field, value)

#     db.commit()
#     db.refresh(tpl)
#     return tpl


# @router.delete("/{template_id}", status_code=status.HTTP_204_NO_CONTENT)
# def delete_email_template(template_id: str, db: Session = Depends(get_db)):
#     tpl = db.query(EmailTemplate).filter(EmailTemplate.id == template_id).first()
#     if not tpl:
#         raise HTTPException(404, "Template not found")

#     db.delete(tpl)
#     db.commit()
#     return


# @router.post("/render/preview", response_model=PreviewResponse)
# def render_preview(payload: PreviewRequest):
#     variables = payload.variables or {}
#     rendered = render_subject_and_body(
#         subject=payload.subject,
#         body=payload.body,
#         variables=variables,
#     )
#     return PreviewResponse(**rendered)


# @router.post("/{template_id}/send-test")
# def send_test_email(
#     template_id: str,
#     payload: SendTestRequest,
#     db: Session = Depends(get_db),
# ):
#     variables = payload.variables or {}

#     try:
#         rendered = render_template_by_id(
#             db=db,
#             template_id=template_id,
#             variables=variables,
#         )
#     except ValueError as e:
#         raise HTTPException(404, str(e))
#     except Exception as e:
#         raise HTTPException(400, f"Render error: {e}")

#     try:
#         send_email_smtp(
#             to=payload.to,
#             subject=rendered["subject"],
#             body=rendered["body"],
#             as_html=rendered.get("is_html", True),
#         )
#     except Exception as e:
#         # ให้ frontend เห็นรายละเอียด SMTP error ด้วย
#         raise HTTPException(500, f"SMTP error: {e}")

#     return {"ok": True, "message": "Test email sent"}

# app/routes/email_template_route.py
from typing import Any, Dict, List, Optional
from datetime import datetime
from uuid import uuid4

from fastapi import APIRouter, Depends, HTTPException, status
from pydantic import BaseModel, EmailStr, ConfigDict
from sqlalchemy.orm import Session
from sqlalchemy.exc import IntegrityError, DataError

from database import get_db
from core.email_smtp import send_email_smtp
from services.email_templates import (
    render_subject_and_body,
    render_template_by_id,
)
from models.email_templates import EmailTemplate

router = APIRouter(prefix="/api/email-templates", tags=["email-templates"])


# ========= SCHEMAS =========

class PreviewRequest(BaseModel):
    subject: str
    body: str
    variables: Dict[str, Any] | None = None


class PreviewResponse(BaseModel):
    subject: str
    body: str


class SendTestRequest(BaseModel):
    """
    ใช้ทั้งส่ง test และส่งจริงให้ client (ขึ้นอยู่กับ frontend ใช้ยังไง)
    """
    to: EmailStr
    variables: Dict[str, Any] | None = None


class EmailTemplateBase(BaseModel):
    slug: str
    name: str
    subject: str
    body: str
    status: str = "Active"
    is_html: bool = True


class EmailTemplateCreate(EmailTemplateBase):
    pass


class EmailTemplateUpdate(BaseModel):
    name: Optional[str] = None
    subject: Optional[str] = None
    body: Optional[str] = None
    status: Optional[str] = None
    is_html: Optional[bool] = None


class EmailTemplateRead(EmailTemplateBase):
    id: str
    updated_at: datetime | None = None

    model_config = ConfigDict(from_attributes=True)


# ========= ENDPOINTS =========

@router.get("/variables/list")
def list_variables():
    """
    ไว้ให้ frontend โชว์ว่าใช้ตัวแปรอะไรได้บ้างใน template
    """
    return {
        "client": [
            "first_name",
            "last_name",
            "email",
            "username",
            "plain_password",
        ],
        "license": [
            "license_key",
            "term",
            "product_sku",
            "expires_at",
        ],
        "meta": [
            "app_name",
            "portal_url",
        ],
    }


@router.get("", response_model=List[EmailTemplateRead])
def list_email_templates(db: Session = Depends(get_db)):
    templates = (
        db.query(EmailTemplate)
        .order_by(EmailTemplate.updated_at.desc())
        .all()
    )
    return templates


@router.post("", response_model=EmailTemplateRead)
def create_template(payload: EmailTemplateCreate, db: Session = Depends(get_db)):
    # check required fields
    for key in ["slug", "name", "subject", "body"]:
        if not getattr(payload, key, None):
            raise HTTPException(status_code=400, detail=f"Missing field: {key}")

    existed = (
        db.query(EmailTemplate)
        .filter(EmailTemplate.slug == payload.slug)
        .first()
    )
    if existed:
        raise HTTPException(400, detail="Slug already exists")

    tpl = EmailTemplate(
        id=str(uuid4()),               # UUID ตรงกับ CHAR(36)
        slug=payload.slug,
        name=payload.name,
        subject=payload.subject,
        body=payload.body,
        status=payload.status,
        is_html=payload.is_html,
    )

    try:
        db.add(tpl)
        db.commit()
        db.refresh(tpl)
    except IntegrityError as e:
        db.rollback()
        raise HTTPException(400, detail=f"Database integrity error: {e.orig}")
    except DataError as e:
        db.rollback()
        raise HTTPException(400, detail=f"Database data error: {e.orig}")

    return tpl


@router.get("/{template_id}", response_model=EmailTemplateRead)
def get_email_template(template_id: str, db: Session = Depends(get_db)):
    tpl = db.query(EmailTemplate).filter(EmailTemplate.id == template_id).first()
    if not tpl:
        raise HTTPException(404, "Template not found")
    return tpl


@router.patch("/{template_id}", response_model=EmailTemplateRead)
def update_email_template(
    template_id: str,
    payload: EmailTemplateUpdate,
    db: Session = Depends(get_db),
):
    tpl = db.query(EmailTemplate).filter(EmailTemplate.id == template_id).first()
    if not tpl:
        raise HTTPException(404, "Template not found")

    data = payload.model_dump(exclude_unset=True)
    for field, value in data.items():
        setattr(tpl, field, value)

    db.commit()
    db.refresh(tpl)
    return tpl


@router.delete("/{template_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_email_template(template_id: str, db: Session = Depends(get_db)):
    tpl = db.query(EmailTemplate).filter(EmailTemplate.id == template_id).first()
    if not tpl:
        raise HTTPException(404, "Template not found")

    db.delete(tpl)
    db.commit()
    return


# ============ PREVIEW ============

@router.post("/render/preview", response_model=PreviewResponse)
def render_preview(payload: PreviewRequest):
    """
    ใช้สำหรับ live preview ในหน้า EditEmail.jsx
    """
    variables = payload.variables or {}
    rendered = render_subject_and_body(
        subject=payload.subject,
        body=payload.body,
        variables=variables,
    )
    return PreviewResponse(**rendered)


# ============ SEND (test / real) ============

@router.post("/{template_id}/send-test")
def send_test_email(
    template_id: str,
    payload: SendTestRequest,
    db: Session = Depends(get_db),
):
    """
    ตอนนี้ใช้เป็น 'send email' จริง ๆ เลย:
      - frontend จะเลือก client จาก dropdown → ใช้ email client เป็น payload.to
      - payload.variables จะถูกส่งมาจาก frontend (client / license / meta)

    แค่ path ชื่อ send-test แต่การทำงานคือส่งจริงเลย
    """
    variables = payload.variables or {}

    # 1) render template จาก DB
    try:
        rendered = render_template_by_id(
            db=db,
            template_id=template_id,
            variables=variables,
        )
    except ValueError as e:
        raise HTTPException(404, str(e))
    except Exception as e:
        raise HTTPException(400, f"Render error: {e}")

    # 2) ส่งเมลผ่าน SMTP
    try:
        send_email_smtp(
            to=payload.to,
            subject=rendered["subject"],
            body=rendered["body"],
            as_html=rendered.get("is_html", True),
        )
    except Exception as e:
        # ให้ frontend เห็นรายละเอียด SMTP error ด้วย (จะได้ debug ง่าย)
        raise HTTPException(500, f"SMTP error: {e}")

    return {"ok": True, "message": f"Email sent to {payload.to}"}
