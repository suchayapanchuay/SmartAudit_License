from database import Base, engine
from models.license import License

Base.metadata.create_all(bind=engine)