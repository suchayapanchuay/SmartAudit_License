
from fastapi import APIRouter, Depends, HTTPException
from fastapi.responses import HTMLResponse
from sqlalchemy.orm import Session
from pydantic import BaseModel
from datetime import datetime, timedelta
import random, string

from database import get_db
from models.license import License

router = APIRouter()

# Request model สำหรับสร้าง License
class LicenseCreateRequest(BaseModel):
    product_name: str
    license_type_id: int | None = None
    user_id: int | None = None
    duration_days: int = 30
    max_activations: int = 3

# ฟังก์ชันสร้าง license key แบบสุ่ม และตรวจสอบซ้ำใน DB
def generate_license_key(db: Session):
    while True:
        key = '-'.join(''.join(random.choices(string.ascii_uppercase + string.digits, k=4)) for _ in range(4))
        exists = db.query(License).filter(License.license_key == key).first()
        if not exists:
            return key

# Endpoint สร้าง License ใหม่
@router.post("/license")
def create_license(req: LicenseCreateRequest, db: Session = Depends(get_db)):
    try:
        license_key = generate_license_key(db)
        now = datetime.utcnow()
        expire = now + timedelta(days=req.duration_days)

        license = License(
            license_key=license_key,
            product_name=req.product_name,
            user_id=req.user_id,
            license_type_id=req.license_type_id,
            issued_date=now,
            expire_date=expire,
            max_activations=req.max_activations,
            activations=0,
            status="active"
        )

        db.add(license)
        db.commit()
        db.refresh(license)

        return {
            "license_key": license.license_key,
            "expire_date": license.expire_date,
            "product_name": license.product_name
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Internal Server Error: {str(e)}")

# Request model สำหรับตรวจสอบ License
class LicenseCheckRequest(BaseModel):
    license_key: str

# Endpoint ตรวจสอบ License (POST)
@router.post("/check_license")
def check_license(
    request: LicenseCheckRequest,
    db: Session = Depends(get_db),
):
    license_key = request.license_key.strip()
    license = db.query(License).filter(License.license_key == license_key).first()

    if not license:
        raise HTTPException(status_code=404, detail="License not found")

    if license.status != "active":
        raise HTTPException(status_code=403, detail="License is not active")

    if license.expire_date and license.expire_date < datetime.utcnow():
        raise HTTPException(status_code=403, detail="License expired")

    return {
        "status": "valid",
        "product": license.product_name,
        "expire_date": license.expire_date,
        "max_activations": license.max_activations,
        "current_activations": license.activations,
    }

@router.get("/check_license", response_class=HTMLResponse)
def check_license_form():
    return """
    <html>
        <head>
            <title>Check License</title>
            <style>
                body {
                    margin: 0;
                    padding: 0;
                    background-color: #f0f4ff;
                    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
                    display: flex;
                    justify-content: center;
                    align-items: center;
                    min-height: 100vh;
                }

                .container {
                    background-color: #ffffff;
                    padding: 2rem 2.5rem;
                    border-radius: 12px;
                    box-shadow: 0 8px 20px rgba(30, 64, 175, 0.15);
                    max-width: 460px;
                    width: 100%;
                    text-align: center;
                }

                .logo {
                    max-width: 120px;
                    margin-bottom: 1.5rem;
                }

                h2 {
                    color: #1e3a8a;
                    margin-bottom: 1.5rem;
                    font-weight: 700;
                    font-size: 1.8rem;
                }

                form {
                    display: flex;
                    gap: 0.5rem;
                    margin-bottom: 1.5rem;
                }

                input[type="text"] {
                    flex-grow: 1;
                    padding: 0.6rem 1rem;
                    font-size: 1rem;
                    border: 2px solid #c7d2fe;
                    border-radius: 6px;
                    outline: none;
                    transition: border-color 0.3s ease;
                    color: #1e3a8a;
                }

                input[type="text"]:focus {
                    border-color: #2563eb;
                    box-shadow: 0 0 8px rgba(37, 99, 235, 0.3);
                }

                button {
                    background-color: #2563eb;
                    color: white;
                    border: none;
                    padding: 0.6rem 1.2rem;
                    font-size: 1rem;
                    border-radius: 6px;
                    font-weight: 600;
                    cursor: pointer;
                    transition: background-color 0.3s ease;
                }

                button:hover {
                    background-color: #1e40af;
                }

                #result {
                    text-align: left;
                    background-color: #eef2ff;
                    border-left: 4px solid #2563eb;
                    padding: 1rem 1.2rem;
                    border-radius: 6px;
                    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.05);
                    font-size: 0.95rem;
                    color: #1e3a8a;
                    word-wrap: break-word;
                }

                #result h3 {
                    margin-top: 0;
                    color: #1e3a8a;
                    margin-bottom: 0.8rem;
                }

                .error {
                    color: #dc2626;
                    font-weight: 600;
                }

                .status-valid {
                    color: #16a34a;
                    font-weight: 700;
                }

                @media (max-width: 480px) {
                    form {
                        flex-direction: column;
                    }

                    input[type="text"], button {
                        width: 100%;
                    }
                }
            </style>
        </head>
        <body>
            <div class="container">
                <img src="/static/logo_smartclick.png" alt="SmartClick Logo" class="logo" />
                <h2>Check License Key</h2>
                <form id="checkForm" autocomplete="off">
                    <input type="text" id="licenseKey" placeholder="Enter license key" required />
                    <button type="submit">Check</button>
                </form>
                <div id="result"></div>
            </div>

            <script>
                const form = document.getElementById("checkForm");
                const resultDiv = document.getElementById("result");

                form.addEventListener("submit", async (e) => {
                    e.preventDefault();
                    const key = document.getElementById("licenseKey").value.trim();

                    if (!key) {
                        resultDiv.innerHTML = '<p class="error">Please enter a license key</p>';
                        return;
                    }

                    resultDiv.innerHTML = '<p>Checking license key...</p>';

                    try {
                        const response = await fetch("/api/check_license", {
                            method: "POST",
                            headers: { "Content-Type": "application/json" },
                            body: JSON.stringify({ license_key: key })
                        });

                        if (!response.ok) {
                            const error = await response.json();
                            resultDiv.innerHTML = `<p class="error">Error: ${error.detail}</p>`;
                            return;
                        }

                        const data = await response.json();
                        let html = `<h3>License Information</h3>
                                   <p><strong>Status:</strong> <span class="status-valid">${data.status}</span></p>
                                   <p><strong>Product:</strong> ${data.product}</p>
                                   <p><strong>Expire Date:</strong> ${new Date(data.expire_date).toLocaleDateString()}</p>
                                   <p><strong>Activations:</strong> ${data.current_activations}/${data.max_activations}</p>`;
                        resultDiv.innerHTML = html;

                    } catch (error) {
                        resultDiv.innerHTML = `<p class="error">Network error: ${error.message}</p>`;
                    }
                });
            </script>
        </body>
    </html>
    """


