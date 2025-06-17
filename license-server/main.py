from fastapi import FastAPI
from routes import license_check, auth_route

app = FastAPI()

@app.get("/")
def root():
    return {"status": "License Server Running"}

# รวม routers
app.include_router(license_check.router, prefix="/api")
app.include_router(auth_route.router, prefix="/api")
