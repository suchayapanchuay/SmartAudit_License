# license-server/routes/order_route.py (หรือไฟล์ใหม่ routes/debug_seed.py)
from fastapi import APIRouter
from utils.events import publish

router = APIRouter()

@router.post("/orders/_debug/seed-trial")
async def seed_trial():
    demo = {
        "type": "order_created",
        "order_code": "TRIAL-" + __import__("secrets").token_hex(3).upper(),
        "name": "Demo User",
        "email": "demo@example.com",
        "meta": {
            "firstName": "Demo",
            "lastName": "User",
            "company": "ACME Co.",
            "industry": "Technology",
            "country": "Thailand",
            "message": "This is a seeded order."
        }
    }
    await publish("order_created", demo)
    return {"seeded": True, "message": "trial order event pushed"}
