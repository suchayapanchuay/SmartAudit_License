from fastapi import Request, HTTPException, status, Depends
from sqlalchemy.orm import Session

from database import get_db
from models.customer import Customer


def get_current_admin(
    request: Request,
    db: Session = Depends(get_db)   # ✅ ถูกต้อง
):
    admin_id = request.session.get("admin_id")

    if not admin_id:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Not authenticated"
        )

    admin = db.query(Customer).filter(Customer.id == admin_id).first()

    if not admin:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Admin not found"
        )

    return admin
