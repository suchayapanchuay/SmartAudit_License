from fastapi import APIRouter, Depends, HTTPException, status
from fastapi.responses import HTMLResponse
from sqlalchemy.orm import Session
from passlib.context import CryptContext
from jose import JWTError, jwt
from pydantic import BaseModel
from datetime import datetime, timedelta
from fastapi.security import OAuth2PasswordBearer
from database import SessionLocal
from models.user import User
import os
from dotenv import load_dotenv

load_dotenv()

router = APIRouter()

# Config
SECRET_KEY = os.getenv("SECRET_KEY", "default-secret")
ALGORITHM = "HS256"
ACCESS_TOKEN_EXPIRE_MINUTES = 30

# Auth helpers
pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")
oauth2_scheme = OAuth2PasswordBearer(tokenUrl="/api/login")

# DB Dependency
def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()

# Pydantic models
class Token(BaseModel):
    access_token: str
    token_type: str

class UserLogin(BaseModel):
    username: str
    password: str

class UserResponse(BaseModel):
    id: int
    username: str

# Password verify
def verify_password(plain, hashed):
    return pwd_context.verify(plain, hashed)

# Create JWT token
def create_access_token(data: dict, expires_delta: timedelta | None = None):
    to_encode = data.copy()
    expire = datetime.utcnow() + (expires_delta or timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES))
    to_encode.update({"exp": expire})
    return jwt.encode(to_encode, SECRET_KEY, algorithm=ALGORITHM)

# Get current user from token
def get_current_user(token: str = Depends(oauth2_scheme), db: Session = Depends(get_db)) -> User:
    credentials_exception = HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Invalid or expired token",
        headers={"WWW-Authenticate": "Bearer"},
    )
    try:
        payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
        username: str = payload.get("sub")
        if not username:
            raise credentials_exception
    except JWTError:
        raise credentials_exception

    user = db.query(User).filter(User.username == username).first()
    if not user:
        raise credentials_exception
    return user

# POST /login
@router.post("/login", response_model=Token)
def login(user: UserLogin, db: Session = Depends(get_db)):
    db_user = db.query(User).filter(User.username == user.username).first()
    if not db_user or not verify_password(user.password, db_user.password_hash):
        raise HTTPException(status_code=400, detail="Invalid credentials")
    access_token = create_access_token(data={"sub": db_user.username})
    return {"access_token": access_token, "token_type": "bearer"}

# GET /logout
@router.get("/logout")
def logout():
    return {"message": "Logout successful (handled client-side)"}

# GET /me (protected)
@router.get("/me", response_model=UserResponse)
def read_me(current_user: User = Depends(get_current_user)):
    return {"id": current_user.id, "username": current_user.username}

# GET /login (HTML form)
@router.get("/login", response_class=HTMLResponse)
def login_form():
    return """
    <html>
        <head><title>Client Login</title></head>
        <body style="font-family: sans-serif; background: #f9f9f9; padding: 2rem;">
            <h2>Client Login</h2>
            <form id="loginForm">
                <input type="text" id="username" placeholder="Username" required /><br><br>
                <input type="password" id="password" placeholder="Password" required /><br><br>
                <button type="submit">Login</button>
            </form>

            <div id="result" style="margin-top: 20px; padding: 15px; border: 1px solid #ccc; border-radius: 5px;"></div>

            <script>
                const form = document.getElementById("loginForm");
                const resultDiv = document.getElementById("result");

                form.addEventListener("submit", async (e) => {
                    e.preventDefault();
                    const username = document.getElementById("username").value.trim();
                    const password = document.getElementById("password").value.trim();

                    if (!username || !password) {
                        resultDiv.innerHTML = "<p style='color:red;'>Please enter both username and password.</p>";
                        return;
                    }

                    try {
                        const response = await fetch("/api/login", {
                            method: "POST",
                            headers: { "Content-Type": "application/json" },
                            body: JSON.stringify({ username, password })
                        });

                        const data = await response.json();

                        if (!response.ok) {
                            resultDiv.innerHTML = `<p style='color:red;'>Login failed: ${data.detail}</p>`;
                            return;
                        }

                        // Save token and redirect
                        localStorage.setItem("access_token", data.access_token);
                        window.location.href = "/api/check_license";

                    } catch (error) {
                        resultDiv.innerHTML = `<p style='color:red;'>Network error: ${error.message}</p>`;
                    }
                });
            </script>
        </body>
    </html>
    """
