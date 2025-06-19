from fastapi import APIRouter, Depends, HTTPException
from fastapi.responses import HTMLResponse
from sqlalchemy.orm import Session
from pydantic import BaseModel
from datetime import datetime

from database import SessionLocal
from models.license import License

router = APIRouter()

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
    license = db.query(License).filter(License.license_key == license_key).first()
    
    if not license:
        raise HTTPException(status_code=404, detail="License not found")
    
    # ตรวจสอบสถานะ license (เปลี่ยนจาก active เป็นตรวจสอบ status)
    if license.status != 'active':
        raise HTTPException(status_code=403, detail="License is not active")
    
    # ตรวจสอบวันหมดอายุ (เปลี่ยนจาก valid_until เป็น expire_date)
    if license.expire_date and license.expire_date < datetime.utcnow():
        raise HTTPException(status_code=403, detail="License expired")
    
    return {
        "status": "valid",
        "product": license.product_name,  # เปลี่ยนจาก product เป็น product_name
        "expire_date": license.expire_date,
        "max_activations": license.max_activations,
        "current_activations": license.activations
    }

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
                <input type="text" id="licenseKey" placeholder="Enter license key" required />
                <button type="submit">Check</button>
            </form>
            <div id="result" style="margin-top: 20px; padding: 15px; border: 1px solid #ddd; border-radius: 5px;"></div>

            <script>
                const form = document.getElementById("checkForm");
                const resultDiv = document.getElementById("result");
                
                form.addEventListener("submit", async (e) => {
                    e.preventDefault();
                    const key = document.getElementById("licenseKey").value.trim();
                    
                    if (!key) {
                        resultDiv.innerHTML = '<p style="color: red;">Please enter a license key</p>';
                        return;
                    }
                    
                    try {
                        const response = await fetch("/check_license", {
                            method: "POST",
                            headers: { "Content-Type": "application/json" },
                            body: JSON.stringify({ license_key: key })
                        });
                        
                        if (!response.ok) {
                            const error = await response.json();
                            resultDiv.innerHTML = `<p style="color: red;">Error: ${error.detail}</p>`;
                            return;
                        }
                        
                        const data = await response.json();
                        let html = `<h3>License Information</h3>
                                   <p><strong>Status:</strong> <span style="color: green;">${data.status}</span></p>
                                   <p><strong>Product:</strong> ${data.product}</p>
                                   <p><strong>Expire Date:</strong> ${new Date(data.expire_date).toLocaleDateString()}</p>
                                   <p><strong>Activations:</strong> ${data.current_activations}/${data.max_activations}</p>`;
                        resultDiv.innerHTML = html;
                        
                    } catch (error) {
                        resultDiv.innerHTML = `<p style="color: red;">Network error: ${error.message}</p>`;
                    }
                });
            </script>
        </body>
    </html>
    """