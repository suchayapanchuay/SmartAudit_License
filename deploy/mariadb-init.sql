-- ========= DATABASE =========
CREATE DATABASE IF NOT EXISTS smartaudit CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE smartaudit;

-- ========= PRODUCTS (รุ่นใหม่) =========
DROP TABLE IF EXISTS products;
CREATE TABLE products (
  id           INT AUTO_INCREMENT PRIMARY KEY,
  sku          VARCHAR(64) NOT NULL UNIQUE,           -- ต้อง NOT NULL เพื่ออ้างอิงด้วย product_sku
  name         VARCHAR(255) NOT NULL UNIQUE,
  category     VARCHAR(100) NULL,
  is_active    TINYINT(1) NOT NULL DEFAULT 1,
  description  TEXT NULL,
  meta         JSON NULL,                             -- เช่น {"durationDays":15,"maxActivations":1}
  created_at   TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ========= CUSTOMERS =========
CREATE TABLE IF NOT EXISTS customers (
  id INT AUTO_INCREMENT PRIMARY KEY,
  email VARCHAR(255) NOT NULL UNIQUE,
  name VARCHAR(255),
  created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- ========= ORDERS =========
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

-- ========= CLIENTS =========
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

-- ========= CLIENT CREDENTIALS =========
CREATE TABLE IF NOT EXISTS client_credentials (
  id INT AUTO_INCREMENT PRIMARY KEY,
  client_id INT NOT NULL,
  username VARCHAR(100) NOT NULL,
  password_hash VARCHAR(255) NOT NULL,
  created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT fk_client_credentials_client FOREIGN KEY (client_id) REFERENCES clients(id) ON DELETE CASCADE,
  UNIQUE KEY uq_client_username (username)
) ENGINE=InnoDB;

-- ========= TRIAL REQUESTS =========
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

-- ========= LICENSES =========
DROP TABLE IF EXISTS licenses;
CREATE TABLE licenses (
  id INT AUTO_INCREMENT PRIMARY KEY,
  client_id INT NOT NULL,
  license_key VARCHAR(64) NOT NULL UNIQUE,
  term VARCHAR(32) NOT NULL DEFAULT 'trial',            -- trial | purchase | support | subscription | perpetual
  product_sku VARCHAR(64) NULL,                         -- อาจ NULL ได้ (เช่น support)
  duration_days INT NULL,
  max_activations INT NOT NULL DEFAULT 1,
  activations_used INT NOT NULL DEFAULT 0,
  status ENUM('active','revoked','expired','inactive') NOT NULL DEFAULT 'active',
  issued_at DATETIME NULL DEFAULT CURRENT_TIMESTAMP,
  expires_at DATETIME NULL,
  created_at DATETIME NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT fk_licenses_client FOREIGN KEY (client_id) REFERENCES clients(id) ON DELETE CASCADE,
  INDEX idx_licenses_client_id (client_id),
  INDEX idx_licenses_key (license_key),
  INDEX idx_licenses_product_sku (product_sku)
) ENGINE=InnoDB;

-- ========= EMAIL TEMPLATES =========
CREATE TABLE IF NOT EXISTS email_templates (
  id          INT AUTO_INCREMENT PRIMARY KEY,
  slug        VARCHAR(64)  NOT NULL UNIQUE,            -- เช่น "welcome"
  name        VARCHAR(255) NOT NULL,                   -- ชื่ออ่านง่าย
  subject     VARCHAR(255) NOT NULL,
  body        MEDIUMTEXT   NOT NULL,                   -- รองรับ HTML ยาว
  is_html     TINYINT(1)   NOT NULL DEFAULT 1,         -- 1 = HTML, 0 = Text
  status      ENUM('Enabled','Disabled') NOT NULL DEFAULT 'Enabled',
  created_at  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at  DATETIME     NULL ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- SEED: welcome template (skip ถ้ามีแล้ว)
INSERT INTO email_templates (slug, name, subject, body, is_html, status)
SELECT 'welcome',
       'Welcome (Default)',
       'Your {{meta.app_name}} Account & License',
       CONCAT(
         '<h3>Welcome {{client.first_name}} {{client.last_name}}</h3>',
         '<p><b>Account</b></p><ul>',
         '<li>Email: {{client.email}}</li>',
         '<li>Username: {{client.username}}</li>',
         '<li>Password: {{client.plain_password}}</li>',
         '</ul>',
         '<p><b>License</b></p><ul>',
         '<li>Key: {{license.license_key}}</li>',
         '<li>Type: {{license.term}}</li>',
         '<li>SKU: {{license.product_sku}}</li>',
         '<li>Expires: {{license.expires_at}}</li>',
         '</ul>',
         '<p>Login: <a href=\"{{meta.portal_url}}\">{{meta.portal_url}}</a></p>'
       ),
       1,
       'Enabled'
WHERE NOT EXISTS (SELECT 1 FROM email_templates WHERE slug='welcome');

-- ========= API KEYS =========
CREATE TABLE IF NOT EXISTS api_keys (
  id               BIGINT AUTO_INCREMENT PRIMARY KEY,
  name             VARCHAR(255) NOT NULL,
  key_prefix       VARCHAR(16)  NOT NULL,
  key_last4        CHAR(4)      NOT NULL,
  key_hash         CHAR(64)     NOT NULL,
  scopes_json      JSON         NOT NULL,
  status           ENUM('active','inactive','revoked') NOT NULL DEFAULT 'active',
  expires_at       DATETIME NULL,
  last_used_at     DATETIME NULL,
  created_by       VARCHAR(255) NULL,
  created_at       DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS api_key_usage (
  id          BIGINT AUTO_INCREMENT PRIMARY KEY,
  api_key_id  BIGINT NOT NULL,
  method      VARCHAR(8) NOT NULL,
  path        VARCHAR(512) NOT NULL,
  status_code INT NOT NULL,
  ip_addr     VARCHAR(64) NULL,
  used_at     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (api_key_id) REFERENCES api_keys(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- ========= ACTIVITY LOGS =========
CREATE TABLE IF NOT EXISTS activity_logs (
  id           INT AUTO_INCREMENT PRIMARY KEY,
  actor        VARCHAR(255) NOT NULL,
  action       VARCHAR(64)  NOT NULL,
  target_type  VARCHAR(64)  NULL,
  target_id    INT          NULL,
  message      VARCHAR(512) NULL,
  ip           VARCHAR(64)  NULL,
  user_agent   VARCHAR(255) NULL,
  meta_json    JSON         NULL,
  created_at   DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- ========= SEED PRODUCTS =========
INSERT INTO products (sku, name, category, is_active, meta)
SELECT 'SMART_AUDIT_TRIAL', 'Smart Audit (Free Trial)', 'license', 1,
       JSON_OBJECT('durationDays', 15, 'maxActivations', 1)
WHERE NOT EXISTS (SELECT 1 FROM products WHERE sku='SMART_AUDIT_TRIAL');

INSERT INTO products (sku, name, category, is_active, meta)
SELECT 'SMART_AUDIT_FULL', 'Smart Audit (Full License)', 'license', 1,
       JSON_OBJECT('durationDays', 365, 'maxActivations', 1)
WHERE NOT EXISTS (SELECT 1 FROM products WHERE sku='SMART_AUDIT_FULL');
