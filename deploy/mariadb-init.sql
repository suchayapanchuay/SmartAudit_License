-- mariadb-init.sql
CREATE DATABASE IF NOT EXISTS smartaudit CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE smartaudit;

-- products ต้องมี sku + ฟิลด์อื่นตามโมเดล
CREATE TABLE IF NOT EXISTS products (
  id INT AUTO_INCREMENT PRIMARY KEY,
  sku VARCHAR(64) NOT NULL UNIQUE,
  name VARCHAR(255) NOT NULL,
  term ENUM('perpetual','subscription') NOT NULL DEFAULT 'subscription',
  duration_months INT NULL,
  max_activations INT NOT NULL DEFAULT 1,
  created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- customers สำหรับเก็บอีเมลผู้สั่ง
CREATE TABLE IF NOT EXISTS customers (
  id INT AUTO_INCREMENT PRIMARY KEY,
  email VARCHAR(255) NOT NULL UNIQUE,
  name VARCHAR(255),
  created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- orders ใช้ TEXT สำหรับ meta เพื่อความคอมพาตกับ MariaDB ทุกเวอร์ชัน
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

-- (ถ้ามีสคีมาเก่าอยู่และอยากเก็บไว้ ก็สามารถสร้างตาราง licenses/activations ตามเดิมเพิ่มได้)

-- seed สินค้าทดลองใช้งาน
INSERT INTO products (sku, name, term, duration_months, max_activations)
SELECT 'SMART_AUDIT_TRIAL', 'Smart Audit (Free Trial)', 'subscription', 1, 1
WHERE NOT EXISTS (SELECT 1 FROM products WHERE sku='SMART_AUDIT_TRIAL');
