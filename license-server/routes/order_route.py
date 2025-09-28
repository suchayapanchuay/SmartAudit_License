# routes/order_route.py
from fastapi import APIRouter, Depends, HTTPException, status
from pydantic import BaseModel, EmailStr, Field
from typing import List, Optional, Any
from sqlalchemy.orm import Session
from sqlalchemy import and_
from datetime import datetime, timezone
from database import get_db
from models.order import Order, OrderStatus
from utils.events import publish

router = APIRouter(prefix="/api/orders", tags=["Orders"])

class OrderItemIn(BaseModel):
    sku: str
    name: str
    qty: int = Field(gt=0)
    price: Optional[float] = None

class OrderIn(BaseModel):
    customer_name: str
    customer_email: EmailStr
    company: Optional[str] = None
    phone: Optional[str] = None
    items: List[OrderItemIn]
    grand_total: Optional[float] = None
    note: Optional[str] = None
    form_type: Optional[str] = "Request"

class OrderOut(BaseModel):
    id: int
    customer_name: str
    customer_email: EmailStr
    company: Optional[str] = None
    phone: Optional[str] = None
    items: List[Any]
    grand_total: Optional[float] = None
    note: Optional[str] = None
    status: OrderStatus
    form_type: str
    created_at: Optional[datetime] = None
    updated_at: Optional[datetime] = None
    class Config: from_attributes = True

@router.get("", response_model=List[OrderOut])
def list_orders(db: Session = Depends(get_db)):
    return (
        db.query(Order)
        .filter(Order.deleted_at.is_(None))
        .order_by(Order.created_at.desc())
        .all()
    )

@router.post("", response_model=OrderOut, status_code=status.HTTP_201_CREATED)
async def create_order(req: OrderIn, db: Session = Depends(get_db)):
    obj = Order(
        customer_name=req.customer_name,
        customer_email=req.customer_email,
        company=req.company,
        phone=req.phone,
        items=[i.model_dump() for i in req.items],
        grand_total=req.grand_total,
        note=req.note,
        form_type=req.form_type or "Request",
        status=OrderStatus.pending,
    )
    db.add(obj); db.commit(); db.refresh(obj)
    out = OrderOut.model_validate(obj).model_dump()
    await publish("order_created", out)
    return obj

@router.delete("/{order_id}")
def delete_order(order_id: int, db: Session = Depends(get_db)):
    row = db.query(Order).filter(and_(Order.id == order_id, Order.deleted_at.is_(None))).first()
    if not row:
        raise HTTPException(status_code=404, detail="Order not found")
    row.deleted_at = datetime.now(timezone.utc)
    db.add(row); db.commit()
    return {"ok": True, "message": "deleted"}

class StatusIn(BaseModel):
    status: OrderStatus

@router.post("/{order_id}/status", response_model=OrderOut)
async def update_status(order_id: int, body: StatusIn, db: Session = Depends(get_db)):
    row = db.query(Order).filter(and_(Order.id == order_id, Order.deleted_at.is_(None))).first()
    if not row:
        raise HTTPException(status_code=404, detail="Order not found")
    row.status = body.status
    db.add(row); db.commit(); db.refresh(row)
    await publish("order_status_changed", {"id": row.id, "status": row.status})
    return row
