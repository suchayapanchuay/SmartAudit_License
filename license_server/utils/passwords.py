from passlib.hash import bcrypt as _bcrypt
def hash_password(p: str) -> str:
    return _bcrypt.hash((p or "")[:72])
