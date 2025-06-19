USE smartaudit;

-- ตารางผู้ใช้ระบบ (Local User เท่านั้น ถ้าไม่ใช้ Web App)
CREATE TABLE users (
    id INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(100) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    is_admin BOOLEAN NOT NULL DEFAULT FALSE,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

-- ตารางประเภท License
CREATE TABLE license_types (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(100) NOT NULL,
    duration_days INT NOT NULL -- ใช้กำหนดอายุ license เช่น 30, 365, 9999 (lifetime)
);

-- ตารางสินค้า / โมดูล / โปรแกรมที่ออก License ได้
CREATE TABLE products (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(255) NOT NULL,
    description TEXT NULL,
    default_max_activations INT NOT NULL DEFAULT 1
);

-- ตาราง License หลัก
CREATE TABLE licenses (
    id INT PRIMARY KEY AUTO_INCREMENT,
    license_key VARCHAR(255) UNIQUE NOT NULL,
    product_name VARCHAR(255) NOT NULL,
    user_id INT NULL,
    license_type_id INT NULL,
    issued_date DATETIME NOT NULL,
    expire_date DATETIME NOT NULL,
    max_activations INT NOT NULL DEFAULT 1,
    activations INT NOT NULL DEFAULT 0,
    status ENUM('active', 'revoked', 'expired') NOT NULL DEFAULT 'active',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE SET NULL,
    FOREIGN KEY (license_type_id) REFERENCES license_types(id) ON DELETE SET NULL
);

-- ตารางเครื่องที่ Activate License
CREATE TABLE activations (
    id INT PRIMARY KEY AUTO_INCREMENT,
    license_id INT NOT NULL,
    machine_id VARCHAR(100) NOT NULL,
    ip_address VARCHAR(45) NULL,
    os_info VARCHAR(255) NULL,
    activated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    last_seen DATETIME NULL,
    status ENUM('active', 'revoked') NOT NULL DEFAULT 'active',
    FOREIGN KEY (license_id) REFERENCES licenses(id) ON DELETE CASCADE,
    UNIQUE KEY (license_id, machine_id)
);

-- ตารางการยกเลิก License (Soft Delete Record)
CREATE TABLE revoked_licenses (
    id INT PRIMARY KEY AUTO_INCREMENT,
    license_id INT NOT NULL,
    revoked_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    reason TEXT NULL,
    FOREIGN KEY (license_id) REFERENCES licenses(id) ON DELETE CASCADE
);

-- ตาราง Log การเปลี่ยนแปลง
CREATE TABLE audit_logs (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NULL,
    action VARCHAR(50) NOT NULL,
    table_name VARCHAR(50) NOT NULL,
    record_id INT NOT NULL,
    old_values TEXT NULL,
    new_values TEXT NULL,
    ip_address VARCHAR(45) NULL,
    user_agent TEXT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE SET NULL
);

-- Indexes เพื่อเพิ่มความเร็ว
CREATE INDEX idx_license_key ON licenses(license_key);
CREATE INDEX idx_license_status ON licenses(status);
CREATE INDEX idx_license_user ON licenses(user_id);
CREATE INDEX idx_activation_license ON activations(license_id);
