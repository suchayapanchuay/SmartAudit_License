# main.py
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import HTMLResponse
from routes.trial_requests import router as trial_router
from routes.admin_notify import router as admin_stream_router
from routes.debug_seed import router as debug_router  
from database import Base,engine
#from routes import license_check, auth_route, license_route, dashboard

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=[
        "http://localhost:3000",      
        "http://127.0.0.1:3000",      
        "http://localhost:3001",      
        "http://127.0.0.1:3001",
    ],
    allow_origin_regex=r"http://(localhost|127\.0\.0\.1):\d+$",
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
                    padding: 3rem 4rem;
                    border-radius: 16px;
                    box-shadow: 0 12px 30px rgba(0,0,0,0.1);
                    max-width: 700px; 
                    width: 100%;
                    text-align: center;
                }
                h1 {
                    margin-bottom: 1rem;
                    color: #2563eb;
                    font-weight: 700;
                    font-size: 2.75rem;
                }
                p {
                    margin: 1rem 0;
                    font-size: 1.2rem;
                    color: #4b5563;
                }
                a {
                    color: #2563eb;
                    text-decoration: none;
                    font-weight: 600;
                    transition: color 0.3s ease;
                }
                a:hover {
                    color: #1d4ed8;
                    text-decoration: underline;
                }
            </style>
        </head>
        <body>
            <div class="container">
                <h1>SmartAudit License Server</h1>
                <p>ระบบ API สำหรับตรวจสอบและจัดการ License ของ SmartAudit</p>
                <p><a href="/docs" target="_blank">Swagger API Docs</a></p>
                <p><a href="/api/check_license" target="_blank">ตรวจสอบ License Key</a></p>
                <p><a href="/api/dashboard" target="_blank">ดูข้อมูล Dashboard (JSON)</a></p>
            </div>
        </body>
    </html>
    """

Base.metadata.create_all(bind=engine)

app.include_router(trial_router)
app.include_router(admin_stream_router)
app.include_router(debug_router)

#app.include_router(license_check.router, prefix="/api", tags=["License"])
#app.include_router(license_route.router, prefix="/api", tags=["License"])
#app.include_router(auth_route.router, prefix="/api", tags=["Auth"])
#app.include_router(dashboard.router, prefix="/api", tags=["Dashboard"])

@app.get("/health")
def health():
    return {"ok": True}


