#from fastapi import APIRouter
#from utils.events import publish
#import secrets
#
#router = APIRouter(prefix="/api", tags=["Debug"])
#
#@router.post("/orders/_debug/seed-trial")
#async def seed_trial():
#    demo = {
#        "id": 0,
#        "customer_name": "Demo User",
#        "customer_email": "demo@example.com",
#        "company": "ACME Co.",
#        "phone": "+66900000000",
#        "items": [{"sku": "CONTACT_SALES", "name": "Contact Sales Lead", "qty": 1}],
#        "grand_total": None,
#        "note": "This is a seeded order.",
#        "status": "pending",
#        "form_type": "Request",
#    }
#    await publish("order_created", demo)
#    return {"seeded": True, "order_code": "TRIAL-" + secrets.token_hex(3).upper()}

from datetime import datetime
from fastapi import APIRouter, Depends
from sqlalchemy.orm import Session

from license_server.database  import get_db
from license_server.models.product import Product

router = APIRouter(prefix="/api", tags=["debug"])

@router.post("/orders/_debug/seed-trial")
def seed_trial_product(db: Session = Depends(get_db)):
    sku = "SMART_AUDIT_TRIAL"
    prod = db.query(Product).filter(Product.sku == sku).first()
    if not prod:
        prod = Product(
            sku=sku,
            name="Smart Audit (Free Trial)",
            term="subscription",
            duration_months=1,
            max_activations=1,
        )
        db.add(prod)
        db.commit()
    return {"ok": True, "product_id": prod.id}
