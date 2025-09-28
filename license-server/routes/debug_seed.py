# routes/debug_seed.py
from fastapi import APIRouter
from utils.events import publish
import secrets

router = APIRouter(prefix="/api", tags=["Debug"])

@router.post("/orders/_debug/seed-trial")
async def seed_trial():
    demo = {
        "id": 0,
        "customer_name": "Demo User",
        "customer_email": "demo@example.com",
        "company": "ACME Co.",
        "phone": "+66900000000",
        "items": [{"sku": "CONTACT_SALES", "name": "Contact Sales Lead", "qty": 1}],
        "grand_total": None,
        "note": "This is a seeded order.",
        "status": "pending",
        "form_type": "Request",
    }
    await publish("order_created", demo)
    return {"seeded": True, "order_code": "TRIAL-" + secrets.token_hex(3).upper()}
