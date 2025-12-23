def success(data: dict):
    return {"success": True, "data": data}

def error(message: str):
    return {"success": False, "error": message}
