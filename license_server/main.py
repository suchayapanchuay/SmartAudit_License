from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import HTMLResponse
from starlette.middleware.sessions import SessionMiddleware

from license_server.database import Base, engine

# โหลดโมเดลก่อน create_all (สำคัญ)
from license_server import models  # noqa: F401

# routers
from license_server.routes.trial_requests import router as trial_router
from license_server.routes.order_route import router as order_router
from license_server.routes.admin_notify import router as admin_stream_router
from license_server.routes.debug_seed import router as debug_router
from license_server.routes.client_route import router as client_router
from license_server.routes.license_route import router as license_router
from license_server.routes.email_template_route import router as email_template_router
from license_server.routes.credentials import router as credentials
from license_server.routes.auth_route import router as auth_router
from license_server.routes.products import router as products_router
from license_server.routes.admin_users import router as admin_users_router
from license_server.routes.dashboard_route import router as dashboard_router
from license_server.routes.reports_route import router as reports_router
from license_server.routes.admin_api_keys import router as api_router
from license_server.routes.health import router as health_router
from license_server.routes.admin_activity_logs import router as activity_router

# events loop capture
import asyncio
from license_server.utils.events import set_main_loop

app = FastAPI(
    docs_url="/docs",
    redoc_url="/redoc",
    openapi_url="/openapi.json",
    swagger_ui_parameters={"displayRequestDuration": True},
)

app.add_middleware(
    SessionMiddleware,
    secret_key="super-secret-key-change-this",
)

@app.on_event("startup")
async def _capture_loop():
    set_main_loop(asyncio.get_running_loop())

app.add_middleware(
    CORSMiddleware,
    allow_origins=[
        "http://localhost:3000",
        "http://127.0.0.1:3000",
        "http://localhost:3001",
        "http://127.0.0.1:3001",
        "http://localhost:5173",
        "http://127.0.0.1:5173",
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
                body { font-family: system-ui, -apple-system, Segoe UI, Roboto; background: #f0f4f8; margin: 0; padding: 2rem; display: flex; justify-content: center; align-items: center; min-height: 100vh; color: #1f2937; }
                .container { background: white; padding: 3rem 4rem; border-radius: 16px; box-shadow: 0 12px 30px rgba(0,0,0,0.1); max-width: 700px; width: 100%; text-align: center; }
                h1 { margin-bottom: 1rem; color: #2563eb; font-weight: 700; font-size: 2.4rem; }
                p { margin: 0.6rem 0; font-size: 1.05rem; color: #4b5563; }
                a { color: #2563eb; text-decoration: none; font-weight: 600; }
                a:hover { color: #1d4ed8; text-decoration: underline; }
            </style>
        </head>
        <body>
            <div class="container">
                <h1>SmartAudit License Server</h1>
                <p>ระบบ API สำหรับตรวจสอบและจัดการ License ของ SmartAudit</p>
                <p><a href="/docs" target="_blank">Swagger API Docs</a></p>
                <p><a href="/api/admin/notifications/stream" target="_blank">Admin SSE Stream</a></p>
            </div>
        </body>
    </html>
    """

# สร้างตาราง (หลัง import models)
Base.metadata.create_all(bind=engine)

# include routers
app.include_router(trial_router)
app.include_router(order_router)
app.include_router(admin_stream_router)
app.include_router(debug_router)
app.include_router(client_router)
app.include_router(license_router)
app.include_router(email_template_router)
app.include_router(credentials)
app.include_router(auth_router)
app.include_router(products_router)
app.include_router(admin_users_router)
app.include_router(dashboard_router)
app.include_router(reports_router)
app.include_router(api_router)
app.include_router(health_router)
app.include_router(activity_router)

@app.get("/health")
def health():
    return {"ok": True}