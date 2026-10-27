PRAGMA foreign_keys = ON;

CREATE TABLE users (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    salt          TEXT NOT NULL,
    nickname      TEXT NOT NULL,
    created_at    TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE categories (
    id        INTEGER PRIMARY KEY AUTOINCREMENT,
    name      TEXT NOT NULL,
    parent_id INTEGER REFERENCES categories(id)
);

CREATE TABLE products (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    seller_id       INTEGER NOT NULL REFERENCES users(id),
    category_id     INTEGER NOT NULL REFERENCES categories(id),
    title           TEXT    NOT NULL,
    description     TEXT,
    price           REAL    NOT NULL CHECK (price >= 0),
    condition_level INTEGER NOT NULL DEFAULT 9,
    status          TEXT    NOT NULL DEFAULT 'ON_SALE',
    created_at      TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
    updated_at      TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE orders (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    order_no   TEXT    NOT NULL UNIQUE,
    product_id INTEGER NOT NULL REFERENCES products(id),
    buyer_id   INTEGER NOT NULL REFERENCES users(id),
    seller_id  INTEGER NOT NULL REFERENCES users(id),
    amount     REAL    NOT NULL,
    status     TEXT    NOT NULL DEFAULT 'PENDING_PAY',
    remark     TEXT,
    created_at TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
    updated_at TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE trade_log (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    order_id    INTEGER NOT NULL REFERENCES orders(id),
    from_status TEXT,
    to_status   TEXT    NOT NULL,
    action      TEXT,
    operator_id INTEGER REFERENCES users(id),
    remark      TEXT,
    created_at  TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE INDEX idx_products_status   ON products(status);
CREATE INDEX idx_products_category ON products(category_id);
CREATE INDEX idx_orders_buyer      ON orders(buyer_id);
CREATE INDEX idx_orders_seller     ON orders(seller_id);
CREATE INDEX idx_trade_log_order   ON trade_log(order_id);

INSERT INTO categories(id, name, parent_id) VALUES
    (1, '数码电子', NULL),
    (2, '图书教材', NULL),
    (3, '家居日用', NULL),
    (4, '服饰鞋包', NULL),
    (5, '运动户外', NULL),
    (6, '手机',     1),
    (7, '电脑',     1),
    (8, '耳机',     1);