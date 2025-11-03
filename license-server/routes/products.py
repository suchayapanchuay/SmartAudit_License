# routers/products.py
from fastapi import APIRouter, Depends, HTTPException, Query, Response
from sqlalchemy.orm import Session
from typing import List, Optional

from database import get_db
from models.product import Product
from schemas.product import ProductCreate, ProductOut, ProductUpdate

router = APIRouter(prefix="/api", tags=["products"])

# --------------------------------------------------------------------
# CREATE
# --------------------------------------------------------------------
@router.post("/products", response_model=ProductOut, status_code=201)
def create_product(payload: ProductCreate, db: Session = Depends(get_db)):
    # กันชื่อซ้ำ
    existing_name = db.query(Product).filter(Product.name == payload.name).first()
    if existing_name:
        raise HTTPException(status_code=409, detail="Product name already exists")

    # กัน SKU ซ้ำ (เฉพาะกรณีมีการกรอก)
    if payload.sku:
        existing_sku = db.query(Product).filter(Product.sku == payload.sku).first()
        if existing_sku:
            raise HTTPException(status_code=409, detail="SKU already exists")

    prod = Product(
        name=payload.name.strip(),
        sku=(payload.sku.strip() if payload.sku else None),
        category=payload.category,
        is_active=payload.isActive,
        description=payload.description,
        meta=(payload.meta.model_dump() if payload.meta else None),
    )
    db.add(prod)
    db.commit()
    db.refresh(prod)

    return ProductOut(
        id=prod.id,
        name=prod.name,
        sku=prod.sku,
        category=prod.category,
        isActive=prod.is_active,
        description=prod.description,
        meta=prod.meta,
        created_at=prod.created_at.isoformat() if prod.created_at else None,
    )

# --------------------------------------------------------------------
# LIST (รองรับ q, limit, offset + ใส่ X-Total-Count)
# --------------------------------------------------------------------
@router.get("/products", response_model=List[ProductOut])
def list_products(
    response: Response,
    q: Optional[str] = Query(None, description="search by name or sku"),
    limit: int = Query(50, ge=1, le=500),
    offset: int = Query(0, ge=0),
    db: Session = Depends(get_db),
):
    base_q = db.query(Product)
    if q:
        like = f"%{q}%"
        base_q = base_q.filter((Product.name.like(like)) | (Product.sku.like(like)))

    total = base_q.count()
    rows = (
        base_q
        .order_by(Product.id.desc())
        .offset(offset)
        .limit(limit)
        .all()
    )

    # ใส่จำนวนทั้งหมดใน header (ช่วย UI ทำ pagination)
    response.headers["X-Total-Count"] = str(total)

    return [
        ProductOut(
            id=r.id,
            name=r.name,
            sku=r.sku,
            category=r.category,
            isActive=r.is_active,
            description=r.description,
            meta=r.meta,
            created_at=r.created_at.isoformat() if r.created_at else None,
        )
        for r in rows
    ]

# --------------------------------------------------------------------
# GET ONE
# --------------------------------------------------------------------
@router.get("/products/{product_id}", response_model=ProductOut)
def get_product(product_id: int, db: Session = Depends(get_db)):
    r = db.get(Product, product_id)  # แทน .get() แบบเก่า
    if not r:
        raise HTTPException(status_code=404, detail="Product not found")

    return ProductOut(
        id=r.id,
        name=r.name,
        sku=r.sku,
        category=r.category,
        isActive=r.is_active,
        description=r.description,
        meta=r.meta,
        created_at=r.created_at.isoformat() if r.created_at else None,
    )

# --------------------------------------------------------------------
# DELETE
# --------------------------------------------------------------------
@router.delete("/products/{product_id}", status_code=204)
def delete_product(product_id: int, db: Session = Depends(get_db)):
    product = db.get(Product, product_id)  # แทน .get() แบบเก่า
    if not product:
        raise HTTPException(status_code=404, detail="Product not found")

    db.delete(product)
    db.commit()
    return  # 204 No Content

# --------------------------------------------------------------------
# UPDATE (PUT /api/products/{product_id})
# --------------------------------------------------------------------
@router.put("/products/{product_id}", response_model=ProductOut)
def update_product(product_id: int, payload: ProductUpdate, db: Session = Depends(get_db)):
    prod = db.get(Product, product_id)
    if not prod:
        raise HTTPException(status_code=404, detail="Product not found")

    # กันชื่อซ้ำ ถ้าส่ง name มาและเปลี่ยนจริง
    if payload.name is not None:
        new_name = payload.name.strip()
        other = db.query(Product).filter(Product.name == new_name, Product.id != product_id).first()
        if other:
            raise HTTPException(status_code=409, detail="Product name already exists")
        prod.name = new_name

    # กัน SKU ซ้ำ ถ้าส่ง sku มาและเปลี่ยนจริง
    if payload.sku is not None:
        new_sku = payload.sku.strip() if payload.sku else None
        if new_sku:
            other = db.query(Product).filter(Product.sku == new_sku, Product.id != product_id).first()
            if other:
                raise HTTPException(status_code=409, detail="SKU already exists")
        prod.sku = new_sku

    if payload.category is not None:
        prod.category = payload.category

    if payload.isActive is not None:
        prod.is_active = payload.isActive

    if payload.description is not None:
        prod.description = payload.description

    # meta เป็น Pydantic model → แปลงเป็น dict ก่อนเก็บลง JSON column
    if payload.meta is not None:
        prod.meta = payload.meta.model_dump()

    db.add(prod)
    db.commit()
    db.refresh(prod)

    return ProductOut(
        id=prod.id,
        name=prod.name,
        sku=prod.sku,
        category=prod.category,
        isActive=prod.is_active,
        description=prod.description,
        meta=prod.meta,
        created_at=prod.created_at.isoformat() if prod.created_at else None,
    )

