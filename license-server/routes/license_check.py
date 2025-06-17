from fastapi import APIRouter, Depends, HTTPException
from fastapi.responses import HTMLResponse
from sqlalchemy.orm import Session
from pydantic import BaseModel
from datetime import datetime

from database import SessionLocal
from models.license import License

router = APIRouter()

# Dependency สำหรับ session ฐานข้อมูล
def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()

class LicenseCheckRequest(BaseModel):
    license_key: str

@router.post("/check_license")
def check_license(request: LicenseCheckRequest, db: Session = Depends(get_db)):
    license_key = request.license_key
    license = db.query(License).filter_by(license_key=license_key).first()
    if not license:
        raise HTTPException(status_code=404, detail="License not found")
    if not license.active:
        raise HTTPException(status_code=403, detail="Inactive license")
    if license.valid_until and license.valid_until < datetime.utcnow():
        raise HTTPException(status_code=403, detail="License expired")
    return {"status": "valid", "product": license.product}

@router.get("/check_license", response_class=HTMLResponse)
def check_license_form():
    return """
    <html>
        <head>
            <title>Check License</title>
        </head>
        <body>
            <h2>Check License Key</h2>
            <form id="checkForm">
                <input type="text" id="licenseKey" placeholder="Enter license key" />
                <button type="submit">Check</button>
            </form>
            <pre id="result"></pre>

            <script>
                const form = document.getElementById("checkForm");
                form.addEventListener("submit", async (e) => {
                    e.preventDefault();
                    const key = document.getElementById("licenseKey").value;
                    const response = await fetch("/api/check_license", {
                        method: "POST",
                        headers: { "Content-Type": "application/json" },
                        body: JSON.stringify({ license_key: key })
                    });
                    const result = await response.json();
                    document.getElementById("result").textContent = JSON.stringify(result, null, 2);
                });
            </script>
        </body>
    </html>
    """
