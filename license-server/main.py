#from fastapi import FastAPI
#from fastapi.middleware.cors import CORSMiddleware
#from fastapi.responses import HTMLResponse
#from database import Base, engine
#
#import models  # noqa: F401
#
#import asyncio
#from utils.events import set_main_loop
#
#from routes.trial_requests import router as trial_router
#from routes.order_route import router as order_router
#from routes.admin_notify import router as admin_stream_router
#from routes.debug_seed import router as debug_router
#from routes.client_route import router as client_router
#
#app = FastAPI()
#
#app.add_middleware(
#    CORSMiddleware,
#    allow_origins=[
#        "http://localhost:3000",
#        "http://127.0.0.1:3000",
#        "http://localhost:3001",
#        "http://127.0.0.1:3001",
#    ],
#    allow_origin_regex=r"http://(localhost|127\.0\.0\.1):\d+$",
#    allow_credentials=True,
#    allow_methods=["*"],
#    allow_headers=["*"],
#)
#
#@app.get("/", response_class=HTMLResponse)
#def root():
#    return """
#    <html>
#        <head>
#            <title>SmartAudit License Server</title>
#            <style>
#                body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif; background: #f0f4f8; margin: 0; padding: 2rem; display: flex; justify-content: center; align-items: center; min-height: 100vh; color: #1f2937; }
#                .container { background: white; padding: 3rem 4rem; border-radius: 16px; box-shadow: 0 12px 30px rgba(0,0,0,0.1); max-width: 700px; width: 100%; text-align: center; }
#                h1 { margin-bottom: 1rem; color: #2563eb; font-weight: 700; font-size: 2.75rem; }
#                p { margin: 1rem 0; font-size: 1.2rem; color: #4b5563; }
#                a { color: #2563eb; text-decoration: none; font-weight: 600; transition: color 0.3s ease; }
#                a:hover { color: #1d4ed8; text-decoration: underline; }
#            </style>
#        </head>
#        <body>
#            <div class="container">
#                <h1>SmartAudit License Server</h1>
#                <p>ระบบ API สำหรับตรวจสอบและจัดการ License ของ SmartAudit</p>
#                <p><a href="/docs" target="_blank">Swagger API Docs</a></p>
#                <p><a href="/api/check_license" target="_blank">ตรวจสอบ License Key</a></p>
#                <p><a href="/api/dashboard" target="_blank">ดูข้อมูล Dashboard (JSON)</a></p>
#            </div>
#        </body>
#    </html>
#    """
#
#Base.metadata.create_all(bind=engine)
#
#app.include_router(trial_router)           # /api/trial-requests
#app.include_router(order_router)           # /api/orders
#app.include_router(admin_stream_router)    # /api/admin/notifications/stream
#app.include_router(debug_router)           # /api/orders/_debug/seed-trial
#app.include_router(client_router)
#
#@app.on_event("startup")
#async def _capture_loop():
#    set_main_loop(asyncio.get_running_loop())
#
#@app.get("/health")
#def health():
#    return {"ok": True}
#

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import HTMLResponse
from database import Base, engine

# โหลดโมเดลก่อน create_all
import models  # noqa: F401

# routers
from routes.trial_requests import router as trial_router
from routes.order_route import router as order_router
from routes.admin_notify import router as admin_stream_router
from routes.debug_seed import router as debug_router
from routes.client_route import router as client_router
from routes.license_route import router as license_router
from routes.email_template_route import router as email_template_router
# ...


# events loop capture
import asyncio
from utils.events import set_main_loop

app = FastAPI()

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
                body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif; background: #f0f4f8; margin: 0; padding: 2rem; display: flex; justify-content: center; align-items: center; min-height: 100vh; color: #1f2937; }
                .container { background: white; padding: 3rem 4rem; border-radius: 16px; box-shadow: 0 12px 30px rgba(0,0,0,0.1); max-width: 700px; width: 100%; text-align: center; }
                h1 { margin-bottom: 1rem; color: #2563eb; font-weight: 700; font-size: 2.75rem; }
                p { margin: 1rem 0; font-size: 1.2rem; color: #4b5563; }
                a { color: #2563eb; text-decoration: none; font-weight: 600; transition: color 0.3s ease; }
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

@app.get("/health")
def health():
    return {"ok": True}
