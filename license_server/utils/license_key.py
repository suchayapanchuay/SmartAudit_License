# license-server/utils/license_key.py
import secrets
import string

ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"  # ตัด 1, I, O, 0

def generate_license_key(groups: int = 4, chars_per_group: int = 5) -> str:
    def chunk():
        return "".join(secrets.choice(ALPHABET) for _ in range(chars_per_group))
    return "-".join(chunk() for _ in range(groups))
