from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles

from routes import license_check, auth_route, license_create

app = FastAPI(
    title="SmartAudit License Server",
    description="API สำหรับตรวจสอบ license และจัดการ auth สำหรับ SmartAudit",
    version="1.0.0"
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/", response_class=HTMLResponse)
def root():
    return """
    <html>
        <head>
            <title>SmartAudit License Server</title>
            <style>
                body {
                    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen,
                        Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif;
                    background: #f0f4f8;
                    margin: 0;
                    padding: 2rem;
                    display: flex;
                    justify-content: center;
                    align-items: center;
                    min-height: 100vh;
                    color: #1f2937;
                }
                .container {
                    background: white;
                    padding: 2.5rem 3rem;
                    border-radius: 12px;
                    box-shadow: 0 10px 25px rgba(0,0,0,0.1);
                    max-width: 480px;
                    width: 100%;
                    text-align: center;
                }
                h1 {
                    margin-bottom: 0.5rem;
                    color: #2563eb; /* blue-600 */
                    font-weight: 700;
                    font-size: 2.5rem;
                }
                p {
                    margin: 0.7rem 0;
                    font-size: 1.1rem;
                    color: #4b5563; /* gray-600 */
                }
                a {
                    color: #2563eb;
                    text-decoration: none;
                    font-weight: 600;
                    transition: color 0.3s ease;
                }
                a:hover {
                    color: #1d4ed8; /* blue-700 */
                    text-decoration: underline;
                }
            </style>
        </head>
        <body>
            <div class="container">
                <h1>License Server Running</h1>
                <p>API สำหรับตรวจสอบ License SmartAudit</p>
                <p>API Docs : <a href="/docs" target="_blank" rel="noopener noreferrer">Swagger UI</a></p>
                <p>Check License Key : <a href="/api/check_license" target="_blank" rel="noopener noreferrer">Check License</a></p>
            </div>
        </body>
    </html>
    """


# Health Check (ใช้สำหรับ Docker, Load Balancer หรือ Monitoring)
@app.get("/health")
def health_check():
    return {"status": "ok"}

# Include Routes
app.include_router(license_check.router, prefix="/api", tags=["License"])
app.include_router(license_create.router, prefix="/api", tags=["License"])
app.include_router(auth_route.router, prefix="/api", tags=["Auth"])
app.mount("/static", StaticFiles(directory="static"), name="static")
