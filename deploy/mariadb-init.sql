USE smartaudit;

CREATE TABLE products (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    description TEXT
);

CREATE TABLE license_types (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100),
    duration_days INT
);

CREATE TABLE licenses (
    id INT AUTO_INCREMENT PRIMARY KEY,
    license_key VARCHAR(255) UNIQUE NOT NULL,
    product_name VARCHAR(255),
    user_id INT,
    license_type_id INT,
    issued_date DATETIME,
    expire_date DATETIME,
    max_activations INT,
    activations INT DEFAULT 0,
    status ENUM('active', 'revoked', 'expired') DEFAULT 'active',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

CREATE TABLE activations (
    id INT AUTO_INCREMENT PRIMARY KEY,
    license_id INT,
    machine_id VARCHAR(100),
    activated_at DATETIME,
    last_seen DATETIME,
    status ENUM('active', 'revoked') DEFAULT 'active',
    FOREIGN KEY (license_id) REFERENCES licenses(id)
);

CREATE TABLE revoked_licenses (
    id INT AUTO_INCREMENT PRIMARY KEY,
    license_id INT,
    revoked_at DATETIME,
    reason TEXT,
    FOREIGN KEY (license_id) REFERENCES licenses(id)
);

CREATE TABLE audit_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    action VARCHAR(255),
    license_id INT,
    user_id INT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    message TEXT
);

