CREATE DATABASE IF NOT EXISTS smartaudit CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE smartaudit;

--products
-- CREATE TABLE IF NOT EXISTS products (
  -- id INT AUTO_INCREMENT PRIMARY KEY,
  -- sku VARCHAR(64) NOT NULL UNIQUE,
  -- name VARCHAR(255) NOT NULL,
  -- term ENUM('perpetual','subscription') NOT NULL DEFAULT 'subscription',
  -- duration_months INT NULL,
  -- max_activations INT NOT NULL DEFAULT 1,
  -- created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP
-- ) ENGINE=InnoDB;

--DROP TABLE IF EXISTS products;
CREATE TABLE products (
  id              INT AUTO_INCREMENT PRIMARY KEY,
  sku             VARCHAR(64) NULL UNIQUE,            -- อนุญาตว่างได้ (หน้า UI ส่ง null ได้)
  name            VARCHAR(255) NOT NULL UNIQUE,       -- กันชื่อซ้ำด้วย
  category        VARCHAR(100) NULL,
  is_active       TINYINT(1) NOT NULL DEFAULT 1,
  description     TEXT NULL,
  meta            JSON NULL,                          -- เก็บ version, licensePolicy, constraints ฯลฯ
  created_at      TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- customers
CREATE TABLE IF NOT EXISTS customers (
  id INT AUTO_INCREMENT PRIMARY KEY,
  email VARCHAR(255) NOT NULL UNIQUE,
  name VARCHAR(255),
  created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- orders
CREATE TABLE IF NOT EXISTS orders (
  id INT AUTO_INCREMENT PRIMARY KEY,
  order_code VARCHAR(64) NOT NULL UNIQUE,
  customer_id INT NOT NULL,
  product_id INT NOT NULL,
  amount_cents INT NOT NULL DEFAULT 0,
  currency VARCHAR(8) DEFAULT 'THB',
  status ENUM('pending','paid','failed','cancelled') NOT NULL DEFAULT 'pending',
  meta TEXT NULL,
  created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT fk_orders_customer FOREIGN KEY (customer_id) REFERENCES customers(id),
  CONSTRAINT fk_orders_product  FOREIGN KEY (product_id)  REFERENCES products(id)
) ENGINE=InnoDB;

-- clients
CREATE TABLE IF NOT EXISTS clients (
  id INT AUTO_INCREMENT PRIMARY KEY,
  request_type ENUM('trial','purchase','support') NOT NULL,
  source VARCHAR(32) NULL,
  source_id VARCHAR(64) NULL,
  first_name VARCHAR(100) NOT NULL,
  last_name  VARCHAR(100) NOT NULL,
  email      VARCHAR(255) NOT NULL,
  phone      VARCHAR(64)  NULL,
  company    VARCHAR(255) NULL,
  industry   VARCHAR(255) NULL,
  country    VARCHAR(128) NULL,
  message    VARCHAR(1000) NULL,
  estimate_user INT NULL,
  trial_days INT NULL,
  created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uq_clients_email (email)
) ENGINE=InnoDB;

-- client_credentials
CREATE TABLE IF NOT EXISTS client_credentials (
  id INT AUTO_INCREMENT PRIMARY KEY,
  client_id INT NOT NULL,
  username VARCHAR(100) NOT NULL,
  password_hash VARCHAR(255) NOT NULL,
  created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT fk_client_credentials_client FOREIGN KEY (client_id) REFERENCES clients(id) ON DELETE CASCADE,
  UNIQUE KEY uq_client_username (username)
) ENGINE=InnoDB;

-- trial_requests
CREATE TABLE IF NOT EXISTS trial_requests (
  id INT AUTO_INCREMENT PRIMARY KEY,
  first_name   VARCHAR(100) NOT NULL,
  last_name    VARCHAR(100) NOT NULL,
  email        VARCHAR(255) NOT NULL,
  phone        VARCHAR(64)  NULL,
  company      VARCHAR(255) NOT NULL,
  industry     VARCHAR(255) NULL,
  country      VARCHAR(128) NOT NULL,
  job_title    VARCHAR(255) NULL,
  message      VARCHAR(1000) NULL,
  utm          TEXT NULL,
  created_at   TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  KEY idx_trial_email (email)
) ENGINE=InnoDB;

-- deploy/mariadb-init.sql (เพิ่มตาราง licenses)
CREATE TABLE IF NOT EXISTS licenses (
  id INT AUTO_INCREMENT PRIMARY KEY,
  client_id INT NOT NULL,
  license_key VARCHAR(64) NOT NULL UNIQUE,
  term VARCHAR(32) NOT NULL DEFAULT 'trial',            -- trial | subscription | perpetual
  product_sku VARCHAR(64) NULL,
  duration_days INT NULL,
  max_activations INT NOT NULL DEFAULT 1,
  activations_used INT NOT NULL DEFAULT 0,
  status VARCHAR(16) NOT NULL DEFAULT 'active',         -- active | revoked | expired
  issued_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  expires_at TIMESTAMP NULL,
  created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT fk_licenses_client FOREIGN KEY (client_id) REFERENCES clients(id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE INDEX IF NOT EXISTS idx_licenses_key ON licenses(license_key);

-- seed
INSERT INTO products (sku, name, term, duration_months, max_activations)
SELECT 'SMART_AUDIT_TRIAL', 'Smart Audit (Free Trial)', 'subscription', 1, 1
WHERE NOT EXISTS (SELECT 1 FROM products WHERE sku='SMART_AUDIT_TRIAL');