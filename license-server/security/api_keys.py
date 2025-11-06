import hashlib, secrets

KEY_PREFIX = "sak_live"  # คุณอาจสลับเป็น sak_test ได้ตาม env

def generate_plain_key(prefix: str = KEY_PREFIX) -> str:
    # prefix_ + random
    return f"{prefix}_{secrets.token_urlsafe(32)}"

def sha256_hex(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()

def mask_from(prefix: str, last4: str) -> str:
    return f"{prefix}_...{last4}"
