from database import SessionLocal
from models.license import License
from datetime import datetime, timedelta
import uuid

def create_license(product, days):
    db = SessionLocal()
    key = str(uuid.uuid4())
    expires = datetime.utcnow() + timedelta(days=days)

    license = License(
        license_key=key,
        product=product,
        valid_until=expires
    )

    db.add(license)
    db.commit()
    db.close()
    print(f"License created: {key}")

if __name__ == "__main__":
    import sys
    p = sys.argv[1] if len(sys.argv) > 1 else "default"
    d = int(sys.argv[2]) if len(sys.argv) > 2 else 30
    create_license(p, d)