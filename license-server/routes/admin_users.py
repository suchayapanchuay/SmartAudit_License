# app/routes/admin_users.py
from fastapi import APIRouter, Depends, Query, HTTPException, status
from pydantic import BaseModel, EmailStr, Field
from typing import Optional, List, Dict
from sqlalchemy.orm import Session
from sqlalchemy import select, or_, func, String
from sqlalchemy.exc import IntegrityError
import os, json, threading

from database import get_db
from models.customer import Customer as AdminUser

router = APIRouter(prefix="/api/admin-users", tags=["Admin Users"])

# ---------- Role (นอกฐานข้อมูล) ----------
class AdminRole(str):
    Administrator = "Administrator"
    Editor = "Editor"
    Viewer = "Viewer"

ROLE_VALUES = {AdminRole.Administrator, AdminRole.Editor, AdminRole.Viewer}

class RoleStore:
    """
    เก็บ role ของ admin users นอกฐานข้อมูลในไฟล์ JSON
    โครงสร้างตัวอย่าง:
    {
      "by_id": {
        "1": "Administrator",
        "2": "Viewer"
      }
    }
    """
    def __init__(self, path: str = None):
        self.path = path or os.getenv("ADMIN_ROLE_STORE", "./var/admin_roles.json")
        os.makedirs(os.path.dirname(self.path), exist_ok=True)
        self._lock = threading.Lock()
        self._data: Dict[str, Dict[str, str]] = {"by_id": {}}
        self._load()

    def _load(self):
        try:
            if os.path.exists(self.path):
                with open(self.path, "r", encoding="utf-8") as f:
                    self._data = json.load(f) or {"by_id": {}}
            else:
                self._save()
        except Exception:
            # ไฟล์พัง? เริ่มใหม่แบบปลอดภัย
            self._data = {"by_id": {}}
            self._save()

    def _save(self):
        tmp = self.path + ".tmp"
        with open(tmp, "w", encoding="utf-8") as f:
            json.dump(self._data, f, ensure_ascii=False, indent=2)
        os.replace(tmp, self.path)

    def get(self, admin_id: int) -> str:
        return self._data.get("by_id", {}).get(str(admin_id), AdminRole.Viewer)

    def set(self, admin_id: int, role: str):
        if role not in ROLE_VALUES:
            raise ValueError("invalid role")
        with self._lock:
            self._data.setdefault("by_id", {})[str(admin_id)] = role
            self._save()

    def delete(self, admin_id: int):
        with self._lock:
            if str(admin_id) in self._data.get("by_id", {}):
                del self._data["by_id"][str(admin_id)]
                self._save()

role_store = RoleStore()

# ---------- Schemas ----------
class AdminUserCreate(BaseModel):
    name: str = Field(..., min_length=1, max_length=255)
    email: EmailStr
    role: Optional[str] = Field(default=AdminRole.Viewer)

class AdminUserUpdate(BaseModel):
    name: Optional[str] = Field(None, min_length=1, max_length=255)
    role: Optional[str] = None

class AdminUserOut(BaseModel):
    id: int
    name: Optional[str] = None
    email: EmailStr
    role: str
    class Config:
        from_attributes = True  # map id/name/email จาก SQLAlchemy model

class PaginatedAdmins(BaseModel):
    items: List[AdminUserOut]
    total: int
    total_pages: int
    page: int
    page_size: int

# ---------- Helpers ----------
def enrich_with_role(rows: List[AdminUser]) -> List[AdminUserOut]:
    out: List[AdminUserOut] = []
    for r in rows:
        out.append(AdminUserOut.model_validate({
            "id": r.id,
            "name": r.name,
            "email": r.email,
            "role": role_store.get(r.id),
        }))
    return out

# ---------- Routes ----------
@router.get("", response_model=PaginatedAdmins)
def list_admin_users(
    db: Session = Depends(get_db),
    search: Optional[str] = Query(None, description="search by name/email"),
    role: Optional[str] = Query(None, description="Administrator|Editor|Viewer"),
    page: int = Query(1, ge=1),
    page_size: int = Query(10, ge=1, le=100),
):
    # 1) query จาก customers
    stmt = select(AdminUser)
    if search:
        s = f"%{search.lower()}%"
        stmt = stmt.where(or_(func.lower(AdminUser.name).like(s),
                              func.lower(AdminUser.email).like(s)))

    # เราต้อง enrich role ก่อนแล้วค่อยกรอง role และ paginate
    rows = db.execute(stmt.order_by(AdminUser.id.desc())).scalars().all()
    enriched = enrich_with_role(rows)

    if role:
        if role not in ROLE_VALUES:
            raise HTTPException(status_code=400, detail="Invalid role")
        enriched = [e for e in enriched if e.role == role]

    total = len(enriched)
    total_pages = max(1, (total + page_size - 1)//page_size) if total else 1

    start = (page - 1) * page_size
    end = start + page_size
    page_items = enriched[start:end]

    return {
        "items": page_items,
        "total": total,
        "total_pages": total_pages,
        "page": page,
        "page_size": page_size,
    }

@router.get("/{admin_id}", response_model=AdminUserOut)
def get_admin(admin_id: int, db: Session = Depends(get_db)):
    user = db.get(AdminUser, admin_id)
    if not user:
        raise HTTPException(status_code=404, detail="Admin not found")
    return AdminUserOut(id=user.id, name=user.name, email=user.email, role=role_store.get(user.id))

@router.post("", response_model=AdminUserOut, status_code=status.HTTP_201_CREATED)
def create_admin(payload: AdminUserCreate, db: Session = Depends(get_db)):
    # insert ลง customers ตาม constraint เดิม (email unique)
    user = AdminUser(name=payload.name, email=payload.email)
    db.add(user)
    try:
        db.commit()
    except IntegrityError:
        db.rollback()
        raise HTTPException(status_code=409, detail="Email already exists")
    db.refresh(user)

    # set role ในไฟล์ sidecar
    role_store.set(user.id, payload.role or AdminRole.Viewer)

    return AdminUserOut(id=user.id, name=user.name, email=user.email, role=role_store.get(user.id))

@router.patch("/{admin_id}", response_model=AdminUserOut)
def update_admin(admin_id: int, payload: AdminUserUpdate, db: Session = Depends(get_db)):
    user = db.get(AdminUser, admin_id)
    if not user:
        raise HTTPException(status_code=404, detail="Admin not found")

    if payload.name is not None:
        user.name = payload.name
    try:
        db.add(user)
        db.commit()
        db.refresh(user)
    except IntegrityError:
        db.rollback()
        raise HTTPException(status_code=400, detail="Update failed")

    if payload.role is not None:
        if payload.role not in ROLE_VALUES:
            raise HTTPException(status_code=400, detail="Invalid role")
        role_store.set(admin_id, payload.role)

    return AdminUserOut(id=user.id, name=user.name, email=user.email, role=role_store.get(user.id))

@router.delete("/{admin_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_admin(admin_id: int, db: Session = Depends(get_db)):
    user = db.get(AdminUser, admin_id)
    if not user:
        raise HTTPException(status_code=404, detail="Admin not found")
    try:
        db.delete(user)
        db.commit()
    except IntegrityError:
        db.rollback()
        # มี FK orders อ้างถึง customers.id อยู่ → ลบไม่ได้
        raise HTTPException(status_code=409, detail="Cannot delete: referenced by orders")

    # ลบ role ออกจากไฟล์ sidecar
    role_store.delete(admin_id)
    return
