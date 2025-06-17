from datetime import datetime

def is_expired(valid_until):
    return valid_until and valid_until < datetime.utcnow()
