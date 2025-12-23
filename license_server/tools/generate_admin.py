from passlib.context import CryptContext

pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")
hashed = pwd_context.hash("1903")  # แก้รหัสตรงนี้
print(hashed)
