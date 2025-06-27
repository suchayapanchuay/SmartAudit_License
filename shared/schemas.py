from pydantic import BaseModel
from datetime import datetime

class LicenseCreateRequest(BaseModel):
    product_name: str
    max_activations: int
    expire_date: datetime
