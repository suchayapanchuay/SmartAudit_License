# routes/order_route.py
from __future__ import annotations

from datetime import datetime
import json
import random
import string
from typing import Any, Dict, List, Optional

from fastapi import APIRouter, Depends, HTTPException, Response, status
from pydantic import BaseModel, EmailStr, Field
from sqlalchemy.orm import Session

from database import get_db
from models.customer import Customer
from models.product import Product
from models.order import Order

from utils.events import publish  # ล้มให้รู้ ถ้า import ไม่ได้

router = APIRouter(prefix="/api", tags=["orders"])

# ---------- Schemas ----------
class OrderItemIn(BaseModel):
    sku: Optional[str] = None
    name: Optional[str] = None
    qty: int = Field(default=1, ge=1)
    price: int = Field(default=0, ge=0)  # หน่วยย่อย (เช่น สตางค์/เซ็นต์)

class OrderIn(BaseModel):
    customer_name: Optional[str] = Field(default=None, max_length=255)
    customer_email: EmailStr
    company: Optional[str] = Field(default=None, max_length=255)
    phone: Optional[str] = Field(default=None, max_length=64)
    items: List[OrderItemIn] = Field(default_factory=list)
    grand_total: int = Field(default=0, ge=0)
    note: Optional[str] = None
    form_type: Optional[str] = Field(default="Purchase", max_length=64)

class OrderOut(BaseModel):
    id: int
    order_code: str
    customer_email: EmailStr
    customer_name: Optional[str]
    company: Optional[str]
    phone: Optional[str]
    items: List[Dict[str, Any]]
    grand_total: int
    currency: str
    status: str
    note: Optional[str]
    created_at: Optional[str]

# ---------- Helpers ----------
def _random_digits(n: int = 10) -> str:
    return "".join(random.choices(string.digits, k=n))

def _gen_order_code() -> str:
    return "ORD-" + _random_digits(10)

def _ensure_unique_order_code(db: Session, max_try: int = 5) -> str:
    for _ in range(max_try):
        code = _gen_order_code()
        exists = db.query(Order).filter(Order.order_code == code).first()
        if not exists:
            return code
    return "ORD-" + _random_digits(14)

def _safe_json_loads(s: Optional[str]) -> Dict[str, Any]:
    try:
        return json.loads(s or "{}")
    except Exception:
        return {}

def _to_out(o: Order) -> OrderOut:
    meta = _safe_json_loads(o.meta)
    return OrderOut(
        id=o.id,
        order_code=o.order_code,
        customer_email=meta.get("customer_email"),
        customer_name=meta.get("customer_name"),
        company=meta.get("company"),
        phone=meta.get("phone"),
        items=meta.get("items", []),
        grand_total=o.amount_cents,
        currency=o.currency or "THB",
        status=o.status,
        note=meta.get("note"),
        created_at=o.created_at.isoformat() if o.created_at else None,
    )

# ---------- Diagnostics ----------
@router.get("/orders/_diag")
def orders_diag():
    return {
        "handler": "v2",
        "auto_fill_items_if_empty": True,
        "build": "2025-11-03",
        "default_form_type": "Purchase",
    }

# ---------- Handlers ----------
@router.post("/orders", response_model=OrderOut, status_code=status.HTTP_201_CREATED)
def create_order_v2(
    body: OrderIn,
    db: Session = Depends(get_db),
    response: Response = None,
):
    if response is not None:
        response.headers["x-order-handler"] = "v2"

    # (B) เติมรายการอัตโนมัติ ถ้าไม่ส่งมา
    if not body.items:
        body.items = [OrderItemIn(sku="PURCHASE_REQUEST", name="Purchase Order Request", qty=1, price=0)]
    first = body.items[0]

    # (C) upsert customer
    cust = db.query(Customer).filter(Customer.email == str(body.customer_email)).first()
    if not cust:
        cust = Customer(
            email=str(body.customer_email),
            name=(body.customer_name or None),
            created_at=datetime.utcnow(),
        )
        db.add(cust)
        db.flush()
    else:
        if body.customer_name and body.customer_name.strip() and (cust.name or "").strip() != body.customer_name.strip():
            cust.name = body.customer_name.strip()

    # (D) find/create product
    prod = None
    if first.sku:
        prod = db.query(Product).filter(Product.sku == first.sku).first()
    if not prod:
        base_sku = first.sku or "PURCHASE_REQUEST"
        candidate = base_sku
        suffix_try = 0
        while db.query(Product).filter(Product.sku == candidate).first():
            suffix_try += 1
            candidate = f"{base_sku}-{suffix_try}"
        gen_name = first.name or "Purchase Order Request"
        prod = Product(
            sku=candidate, name=gen_name, category="purchase", is_active=True,
            description="Auto-created from purchase form",
            meta={"source": "purchase_form"},
            created_at=datetime.utcnow(),
        )
        db.add(prod)
        db.flush()

    # (E) amount
    amount_cents = int(body.grand_total or 0)
    if amount_cents == 0:
        try:
            amount_cents = int(first.price or 0) * int(first.qty or 1)
        except Exception:
            amount_cents = 0

    # (F) meta
    items_dump = [{"sku": x.sku, "name": x.name, "qty": x.qty, "price": x.price} for x in body.items]
    meta: Dict[str, Any] = {
        "customer_email": str(body.customer_email),
        "customer_name": body.customer_name,
        "company": body.company,
        "phone": body.phone,
        "items": items_dump,
        "grand_total": amount_cents,
        "note": body.note,
        "form_type": (body.form_type or "Purchase"),
        "source": "purchase_form",
        "is_trial": False,
    }

    # (G) order_code
    order_code = _ensure_unique_order_code(db)

    # (H) create
    o = Order(
        order_code=order_code,
        customer_id=cust.id,
        product_id=prod.id,
        amount_cents=amount_cents,
        currency="THB",
        status="pending",
        meta=json.dumps(meta, ensure_ascii=False),
        created_at=datetime.utcnow(),
    )

    try:
        db.add(o)
        db.commit()
    except Exception as e:
        db.rollback()
        raise HTTPException(status_code=500, detail=f"Failed to create order: {e}") from e

    db.refresh(o)

    # (I) events
    try:
        publish("order_created", {
            "id": o.id,
            "order_code": o.order_code,
            "customer_email": str(body.customer_email),
            "customer_name": body.customer_name,
            "company": body.company,
            "phone": body.phone,
            "items": items_dump,
            "grand_total": o.amount_cents,
            "currency": o.currency or "THB",
            "status": o.status,
            "note": body.note,
            "created_at": o.created_at.isoformat() if o.created_at else None,
        })

        # จะคงไว้ใช้แจ้ง banner/โต๊ะรวมก็ได้
        publish("admin_notify", {
            "type": "purchase",
            "title": "New Purchase Request",
            "message": f"{body.customer_name or '-'} • {body.company or '-'}",
            "email": str(body.customer_email),
            "phone": body.phone,
            "company": body.company,
            "form_type": (body.form_type or "Purchase"),
            "order_id": o.id,
            "order_code": o.order_code,
            "product_sku": (first.sku or ""),
            "amount_cents": o.amount_cents,
            "currency": o.currency or "THB",
            "at": o.created_at.isoformat() + 'Z' if o.created_at else None,
        })
    except Exception:
        pass

    return _to_out(o)

@router.get("/orders", response_model=List[OrderOut])
def list_orders(db: Session = Depends(get_db)):
    q = db.query(Order).order_by(Order.id.desc()).all()
    return [_to_out(x) for x in q]

@router.delete("/orders/{order_id}")
def delete_order(order_id: int, db: Session = Depends(get_db)):
    obj = db.get(Order, order_id)
    if not obj:
        raise HTTPException(status_code=404, detail="Not found")
    db.delete(obj)
    db.commit()
    return {"ok": True}
