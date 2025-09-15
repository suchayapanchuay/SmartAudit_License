# routes/debug_seed.py
from fastapi import APIRouter
from utils.events import publish
import secrets, time

router = APIRouter()

@router.post("/orders/_debug/seed-trial")
async def seed_trial():
    demo = {
        "order_code": "TRIAL-" + secrets.token_hex(3).upper(),
        "name": "Demo User",
        "email": "demo@example.com",
        "meta": {
            "firstName": "Demo",
            "lastName": "User",
            "company": "ACME Co.",
            "industry": "Technology",
            "country": "Thailand",
            "message": "This is a seeded order.",
        },
        "ts": time.time(),
    }
    await publish("order_created", demo)
    return {"seeded": True, "message": "trial order event pushed"}
