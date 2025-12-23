# schemas/product.py
from typing import List, Optional, Any, Dict
from pydantic import BaseModel, Field

class LicensePolicy(BaseModel):
    supportedTypes: List[str] = Field(default_factory=list)   # ["trial","subscription","perpetual"]
    durationDays: Optional[int] = None

class Constraints(BaseModel):
    maxSeats: Optional[int] = None
    maxDevice: Optional[int] = None
    rateLimit: Optional[str] = None

class ProductMeta(BaseModel):
    version: Optional[str] = None
    licensePolicy: Optional[LicensePolicy] = None
    constraints: Optional[Constraints] = None
    _searchEcho: Optional[str] = None

class ProductCreate(BaseModel):
    name: str
    sku: Optional[str] = None
    category: Optional[str] = None
    isActive: bool = True
    description: Optional[str] = None
    meta: Optional[ProductMeta] = None

class ProductUpdate(BaseModel):
    name: Optional[str] = None
    sku: Optional[str] = None
    category: Optional[str] = None
    isActive: Optional[bool] = None
    description: Optional[str] = None
    meta: Optional[ProductMeta] = None

class ProductOut(BaseModel):
    id: int
    name: str
    sku: Optional[str] = None
    category: Optional[str] = None
    isActive: bool
    description: Optional[str] = None
    meta: Optional[Dict[str, Any]] = None
    created_at: Optional[str] = None

    class Config:
        from_attributes = True
