#import json
#import random
#from datetime import datetime
#from typing import Optional, List, Union
#from fastapi import APIRouter, Depends, HTTPException
#from pydantic import BaseModel, EmailStr, field_validator
#from sqlalchemy.orm import Session
#
#from database import get_db
#from models.customer import Customer
#from models.product import Product
#from models.order import Order
#from utils.events import publish
#
#router = APIRouter(prefix="/api", tags=["orders"])
#
#class OrderItemIn(BaseModel):
#    sku: str
#    name: Optional[str] = None
#    qty: int = 1
#    price: Union[int, float] = 0
#
#    @field_validator("sku", mode="before")
#    @classmethod
#    def _sku_required(cls, v):
#        s = str(v or "").strip()
#        if not s:
#            raise ValueError("sku is required")
#        return s
#
#    @field_validator("qty", mode="before")
#    @classmethod
#    def _qty_pos(cls, v):
#        try:
#            n = int(v)
#        except Exception:
#            raise ValueError("qty must be int")
#        if n < 1:
#            raise ValueError("qty must be >= 1")
#        return n
#
#class OrderCreateIn(BaseModel):
#    customer_name: str
#    customer_email: EmailStr
#    company: Optional[str] = None
#    phone: Optional[str] = None
#    items: List[OrderItemIn]
#    grand_total: Optional[Union[int, float]] = None
#    note: Optional[str] = None
#    form_type: Optional[str] = None
#
#    @field_validator("customer_name", mode="before")
#    @classmethod
#    def _name_strip(cls, v):
#        s = str(v or "").strip()
#        if not s:
#            raise ValueError("customer_name required")
#        return s
#
#class OrderOut(BaseModel):
#    id: int
#    order_code: str
#
#def _price_to_cents(v: Optional[Union[int, float]]) -> int:
#    if v is None:
#        return 0
#    try:
#        return int(round(float(v) * 100))
#    except Exception:
#        try:
#            return int(v)
#        except Exception:
#            return 0
#
#def _gen_order_code() -> str:
#    ts = datetime.utcnow().strftime("%Y%m%d")
#    rnd = random.randint(1000, 9999)
#    return f"ORD-{ts}-{rnd}"
#
#@router.post("/orders", response_model=OrderOut, status_code=201)
#async def create_order(body: OrderCreateIn, db: Session = Depends(get_db)):
#    if not body.items:
#        raise HTTPException(status_code=400, detail="items must not be empty")
#
#    item = body.items[0]
#
#    # upsert customer
#    cust = db.query(Customer).filter(Customer.email == str(body.customer_email)).first()
#    if not cust:
#        cust = Customer(email=str(body.customer_email), name=body.customer_name.strip())
#        db.add(cust)
#        db.flush()
#    else:
#        if body.customer_name and body.customer_name.strip() and cust.name != body.customer_name.strip():
#            cust.name = body.customer_name.strip()
#
#    # find/create product
#    prod = db.query(Product).filter(Product.sku == item.sku).first()
#    if not prod:
#        prod = Product(
#            sku=item.sku,
#            name=item.name or item.sku,
#            term="subscription",
#            duration_months=12,
#            max_activations=1,
#        )
#        db.add(prod)
#        db.flush()
#
#    amount_cents = (
#        _price_to_cents(body.grand_total)
#        if body.grand_total is not None
#        else _price_to_cents(item.price) * int(item.qty)
#    )
#
#    try:
#        order = Order(
#            order_code=_gen_order_code(),
#            customer_id=cust.id,
#            product_id=prod.id,
#            amount_cents=amount_cents,
#            currency="THB",
#            status="pending",
#            meta=json.dumps({
#                "company": body.company,
#                "phone": body.phone,
#                "items": [i.model_dump() for i in body.items],
#                "grand_total": body.grand_total,
#                "note": body.note,
#                "form_type": body.form_type,
#                "source": "public_site",
#            }, ensure_ascii=False),
#            created_at=datetime.utcnow(),
#        )
#        db.add(order)
#        db.commit()
#        db.refresh(order)
#    except Exception as e:
#        db.rollback()
#        raise HTTPException(status_code=500, detail=f"DB error: {e}")
#
#    publish("order_created", {
#        "id": order.id,
#        "order_code": order.order_code,
#        "customer_email": cust.email,
#        "customer_name": cust.name,
#        "product_sku": prod.sku,
#        "amount_cents": order.amount_cents,
#        "created_at": order.created_at.isoformat() if order.created_at else None,
#    })
#
#    return OrderOut(id=order.id, order_code=order.order_code)

from datetime import datetime
import json
import random
import string
from typing import Any, Dict, List, Optional

from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel, EmailStr
from sqlalchemy.orm import Session

from database import get_db
from models.customer import Customer
from models.product import Product
from models.order import Order
from utils.events import publish

router = APIRouter(prefix="/api", tags=["orders"])

# ---- Schemas ----
class OrderItemIn(BaseModel):
    sku: Optional[str] = None
    name: Optional[str] = None
    qty: int = 1
    price: int = 0

class OrderIn(BaseModel):
    customer_name: Optional[str] = None
    customer_email: EmailStr
    company: Optional[str] = None
    phone: Optional[str] = None
    items: List[OrderItemIn] = []
    grand_total: int = 0
    note: Optional[str] = None
    form_type: Optional[str] = None

class OrderOut(BaseModel):
    id: int
    customer_email: EmailStr
    customer_name: Optional[str]
    company: Optional[str]
    phone: Optional[str]
    items: List[Dict[str, Any]]
    grand_total: int
    status: str
    note: Optional[str]
    created_at: Optional[str]

def _code() -> str:
    return "ODR" + "".join(random.choices(string.digits, k=9))

def _to_out(o: Order) -> OrderOut:
    meta = {}
    try:
        meta = json.loads(o.meta or "{}")
    except Exception:
        pass
    return OrderOut(
        id=o.id,
        customer_email=meta.get("customer_email"),
        customer_name=meta.get("customer_name"),
        company=meta.get("company"),
        phone=meta.get("phone"),
        items=meta.get("items", []),
        grand_total=o.amount_cents,
        status=o.status,
        note=meta.get("note"),
        created_at=o.created_at.isoformat() if o.created_at else None,
    )

@router.post("/orders", response_model=OrderOut)
def create_order(body: OrderIn, db: Session = Depends(get_db)):
    # 1) customer
    cust = db.query(Customer).filter(Customer.email == str(body.customer_email)).first()
    if not cust:
        cust = Customer(email=str(body.customer_email), name=(body.customer_name or None), created_at=datetime.utcnow())
        db.add(cust)
        db.flush()

    # 2) product (ใช้ item ตัวแรกเป็นตัวแทน order.product_id)
    if not body.items:
        raise HTTPException(status_code=400, detail="items required")
    first = body.items[0]
    prod = None
    if first.sku:
        prod = db.query(Product).filter(Product.sku == first.sku).first()
    if not prod:
        # auto-create ถ้าไม่พบ
        prod = Product(
            sku=first.sku or ("SKU" + "".join(random.choices(string.ascii_uppercase + string.digits, k=8))),
            name=first.name or "Custom Product",
            term="subscription",
            duration_months=1,
            max_activations=1,
        )
        db.add(prod)
        db.flush()

    # 3) order
    o = Order(
        order_code=_code(),
        customer_id=cust.id,
        product_id=prod.id,
        amount_cents=int(body.grand_total or 0),
        currency="THB",
        status="pending",
        meta=json.dumps({
            "customer_email": str(body.customer_email),
            "customer_name": body.customer_name,
            "company": body.company,
            "phone": body.phone,
            "items": [x.model_dump() for x in body.items],
            "note": body.note,
            "form_type": body.form_type,
        }, ensure_ascii=False),
        created_at=datetime.utcnow(),
    )
    db.add(o)
    db.commit()
    db.refresh(o)

    publish("order_created", {
        "id": o.id,
        "customer_email": body.customer_email,
        "customer_name": body.customer_name,
        "company": body.company,
        "phone": body.phone,
        "items": [x.model_dump() for x in body.items],
        "grand_total": o.amount_cents,
        "status": o.status,
        "note": body.note,
        "created_at": o.created_at.isoformat() if o.created_at else None,
    })

    return _to_out(o)

@router.get("/orders", response_model=List[OrderOut])
def list_orders(db: Session = Depends(get_db)):
    q = db.query(Order).order_by(Order.id.desc()).all()
    return [_to_out(x) for x in q]

@router.delete("/orders/{order_id}")
def delete_order(order_id: int, db: Session = Depends(get_db)):
    o = db.get(Order, order_id)
    if not o:
        raise HTTPException(status_code=404, detail="Not found")
    db.delete(o)
    db.commit()
    return {"ok": True}

