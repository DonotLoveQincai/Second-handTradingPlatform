# 任务表 A · 商品域（上架 / 修改 / 下架 / 检索）

> **一句话**：把商品能发布出去、能搜到、能下架。
> **你不用懂**：登录怎么做的（B）、状态怎么流转（C）、窗口怎么搭的（D）。
> **你依赖 D**：`DbManager`、`OpResult`、`Const::`、`MainWindow` 的挂载点。
> **别人依赖你**：B 下单要调你的 `updateStatus`；C 取消订单要调你的 `updateStatus`。

---

## 你要改的文件（只有这些）

```
core/entity/product.h
core/repository/i_product_repository.h
data/product_dao.h  /  .cpp
services/product_service.h  /  .cpp
ui/pages/product/search_page.h / .cpp          ← 检索页（默认页）
ui/pages/product/publish_dialog.h / .cpp       ← 发布对话框
ui/pages/product/my_products_page.h / .cpp     ← 我的商品页
```

---

## 任务清单

| # | 任务 | 周 | 前置 | 完成 |
|---|---|---|---|---|
| 1 | 环境跑通，开分支 `feature/A-product` | 0 | 装好 Qt + VS | ☐ |
| 2 | `product.h`（实体 + 查询条件） | 0 | 读过契约 §6 | ☐ |
| 3 | `i_product_repository.h` | 0 | 2 | ☐ |
| 4 | 提 PR 让 D review | 0 | 3 | ☐ |
| 5 | `ProductDao::insert` | 1 | D 的建表 SQL | ☐ |
| 6 | `ProductDao::updateStatus`（三参数条件更新）+ `updateInfo`（改信息） | 1 | 5 | ☐ |
| 7 | `ProductDao::findById` / `findBySeller` | 1 | 5 | ☐ |
| 8 | `ProductDao::search`（关键字 + 分类 + 价格 + 排序） | 1 | 5 | ☐ |
| 9 | `ProductService::publish` / `updateInfo` / `takeDown` | 1 | 5,6 | ☐ |
| 10 | 检索页 `SearchPage` | 2 | 8 | ☐ |
| 11 | 发布对话框 `PublishDialog`（兼编辑框） | 2 | 9 | ☐ |
| 12 | 我的商品页 `MyProductsPage`（上架/下架/修改按钮） | 2 | 6,7 | ☐ |
| 13 | 双击商品 → 发信号 `buyRequested(productId)` | 2 | 10 | ☐ |
| 14 | 和 B 联调：买你的商品能下单成功 | 3 | 13 | ☐ |

---

## 第 0 周

### 1. 分支

```bash
git checkout develop && git pull
git checkout -b feature/A-product
```

### 2. `core/entity/product.h`

照契约 §6 抄 `Product` 和 `ProductQuery` 两个 struct，**一个字别改**。

### 3. `core/repository/i_product_repository.h`

```cpp
#pragma once
#include "core/entity/product.h"
#include "core/common/opresult.h"
#include <optional>

class IProductRepository {
public:
    virtual ~IProductRepository() = default;
    virtual OpResult insert(Product& p) = 0;
    virtual OpResult updateStatus(int id, const QString& from, const QString& to) = 0;
    virtual OpResult updateInfo(const Product& p) = 0;      // 改标题/分类/描述/价格/成色
    virtual std::optional<Product> findById(int id) = 0;
    virtual QVector<Product> search(const ProductQuery& q) = 0;
    virtual QVector<Product> findBySeller(int sellerId) = 0;
};
```

### 4. 提 PR → Reviewer 选 D

---

## 第 1 周（先跑通数据层，界面下周再说）

### 5. `ProductDao::insert`

```cpp
OpResult ProductDao::insert(Product& p)
{
    QSqlQuery q(DbManager::db());
    q.prepare("INSERT INTO products(seller_id,category_id,title,description,price,"
              "condition_level,status) VALUES(?,?,?,?,?,?,?)");
    q.addBindValue(p.sellerId);
    q.addBindValue(p.categoryId);
    q.addBindValue(p.title);
    q.addBindValue(p.description);
    q.addBindValue(p.price);
    q.addBindValue(p.conditionLevel);
    q.addBindValue(Const::ProductStatus::OnSale);
    if (!q.exec()) return OpResult::fail("发布失败：" + q.lastError().text());
    p.id = q.lastInsertId().toInt();
    return OpResult::success(QString::number(p.id));
}
```

### 6. `ProductDao::updateStatus` —— **整个项目最关键的 8 行**

```cpp
OpResult ProductDao::updateStatus(int id, const QString& from, const QString& to)
{
    QSqlQuery q(DbManager::db());
    q.prepare("UPDATE products SET status=?, updated_at=datetime('now','localtime') "
              "WHERE id=? AND status=?");
    q.addBindValue(to);       // 第 1 个 ?
    q.addBindValue(id);       // 第 2 个 ?
    q.addBindValue(from);     // 第 3 个 ?
    if (!q.exec()) return OpResult::fail("更新商品状态失败：" + q.lastError().text());
    if (q.numRowsAffected() != 1)
        return OpResult::fail("商品状态已变，请刷新后重试");
    return OpResult::success();
}
```

四种用法（别写新函数）：

| 用途 | 参数 | 谁调 |
|---|---|---|
| 上架 | `(id, OFF_SALE, ON_SALE)` | 你 |
| 下架 | `(id, ON_SALE, OFF_SALE)` | 你 |
| 下单占用 | `(id, ON_SALE, SOLD)` | B |
| 取消释放 | `(id, SOLD, ON_SALE)` | C |

✅ **验证**：同一条记录连调两次 `updateStatus(id, ON_SALE, SOLD)`，第二次必须返回失败。

### 6.1 `ProductDao::updateInfo`（改信息）

改信息**不动 status**，也别用 `if (numRowsAffected() != 1)` 拦——SQLite 在"新值和旧值完全一样"时
`numRowsAffected()` 可能返回 1、也可能返回 0，各版本不一致，拿它做判断会出现"我什么都没改却报错"。
这里只判断 `exec()` 成不成功：

```cpp
OpResult ProductDao::updateInfo(const Product& p)
{
    QSqlQuery q(DbManager::db());
    q.prepare("UPDATE products SET title=?, category_id=?, description=?, "
              "price=?, condition_level=?, updated_at=datetime('now','localtime') "
              "WHERE id=? AND status<>'SOLD'");      // 已售出的连 SQL 都改不动
    q.addBindValue(p.title);
    q.addBindValue(p.categoryId);
    q.addBindValue(p.description);
    q.addBindValue(p.price);
    q.addBindValue(p.conditionLevel);
    q.addBindValue(p.id);
    if (!q.exec()) return OpResult::fail("修改商品失败：" + q.lastError().text());
    return OpResult::success();
}
```

> `WHERE ... AND status<>'SOLD'` 这句是"数据库层的最后一道门"：即使界面上的按钮忘了灰掉，已售出的商品也改不了。

✅ **验证**：拿一个 SOLD 的商品 id 调 `updateInfo`，改完 `SELECT title` 看，标题必须没变。

### 7. `findById` / `findBySeller`

```cpp
// 两条 SQL 都带这个 JOIN，界面才能显示卖家和分类名字
"SELECT p.*, u.nickname AS seller_name, c.name AS category_name "
"FROM products p "
"LEFT JOIN users u ON u.id = p.seller_id "
"LEFT JOIN categories c ON c.id = p.category_id "
```

取值时写 `q.value("seller_name")`（**用列名，不要用序号**，以后加列不会错位）。
`findBySeller` 就是加 `WHERE p.seller_id = ? ORDER BY p.created_at DESC`。

### 8. `search`

只拼结构、不拼值：

```cpp
QString buildWhere(const ProductQuery& q, QVariantList& binds)
{
    QString sql = " WHERE p.status = 'ON_SALE'";
    if (!q.keyword.isEmpty()) {
        sql += " AND p.title LIKE ?";
        binds << "%" + q.keyword + "%";
    }
    if (!q.categoryIds.isEmpty()) {
        QStringList ph;                         // 生成 ?,?,?
        for (int i = 0; i < q.categoryIds.size(); ++i) ph << "?";
        sql += " AND p.category_id IN (" + ph.join(",") + ")";
        for (int id : q.categoryIds) binds << id;
    }
    if (q.priceMin >= 0) { sql += " AND p.price >= ?"; binds << q.priceMin; }
    if (q.priceMax >= 0) { sql += " AND p.price <= ?"; binds << q.priceMax; }
    return sql;
}
```

排序**只查白名单**（用户输入绝不能拼进 SQL）：

```cpp
static const QHash<QString,QString> orderMap = {
    {"time_desc","p.created_at DESC"}, {"price_asc","p.price ASC"}, {"price_desc","p.price DESC"}
};
sql += " ORDER BY " + orderMap.value(q.sortBy, "p.created_at DESC");
```

✅ **验证**：只传关键字能搜到；分类 + 价格区间一起用也准；已下架/已售出的搜不到（因为写死了 `ON_SALE`）。

### 9. `ProductService`

```cpp
OpResult ProductService::publish(Product& p, int sellerId)
{
    p.title = p.title.trimmed();
    if (p.title.isEmpty())          return OpResult::fail("请填写标题");
    if (p.title.size() > 60)        return OpResult::fail("标题不能超过 60 字");
    if (p.price <= 0)               return OpResult::fail("价格必须大于 0");
    if (p.categoryId <= 0)          return OpResult::fail("请选择分类");
    if (p.conditionLevel < 1 || p.conditionLevel > 10)
        return OpResult::fail("成色只能是 1~10");
    p.sellerId = sellerId;
    auto r = m_repo->insert(p);
    if (r.ok) emit productsChanged();
    return r;
}

OpResult ProductService::updateInfo(Product p, int sellerId)
{
    auto old = m_repo->findById(p.id);
    if (!old)                          return OpResult::fail("商品不存在");
    if (old->sellerId != sellerId)     return OpResult::fail("只能修改自己发布的商品");
    if (old->status == Const::ProductStatus::Sold)
        return OpResult::fail("已售出的商品不能修改");

    p.title = p.title.trimmed();
    if (p.title.isEmpty())             return OpResult::fail("请填写标题");
    if (p.title.size() > 60)           return OpResult::fail("标题不能超过 60 字");
    if (p.price <= 0)                  return OpResult::fail("价格必须大于 0");
    if (p.categoryId <= 0)             return OpResult::fail("请选择分类");
    if (p.conditionLevel < 1 || p.conditionLevel > 10)
        return OpResult::fail("成色只能是 1~10");

    p.sellerId = sellerId;                 // 卖家不允许被改
    auto r = m_repo->updateInfo(p);
    if (r.ok) emit productsChanged();
    return r;
}

OpResult ProductService::takeDown(int productId, int sellerId)
{
    auto p = m_repo->findById(productId);
    if (!p)                        return OpResult::fail("商品不存在");
    if (p->sellerId != sellerId)   return OpResult::fail("只能下架自己发布的商品");
    auto r = m_repo->updateStatus(productId, Const::ProductStatus::OnSale,
                                             Const::ProductStatus::OffSale);
    if (r.ok) emit productsChanged();
    return r;
}
```

---

## 第 2 周（三个界面，都用最土的 QTableWidget）

### 10. 检索页

```
┌──────────┬──────────────────────────────────────────────┐
│ 分类      │ 关键字[______] 价格[__]-[__] 排序[▾] [搜索] [发布商品]│
│ 全部      ├──────────────────────────────────────────────┤
│ 数码电子  │ 标题 │ 分类 │ 价格 │ 成色 │ 卖家 │ 发布时间  │
│   手机    │ ... 表格，双击一行 = 想买                    │
│   电脑    │                                              │
│ 图书教材  │ 共 12 件                                     │
└──────────┴──────────────────────────────────────────────┘
```

- 左侧 `QListWidget` 装分类：`全部` 一项 + 5 个父分类 + 子分类（用 `item->setData(Qt::UserRole, id)` 存 id）
- 点父分类时把它的子分类 id 一起塞进 `categoryIds`
- 右侧 `QTableWidget`，8 列够用，`setEditTriggers(NoEditTriggers)` 设成只读
- 底部一行 `QLabel` 显示"共 N 件"（`table->rowCount()` 就行，**不用再查一次 count**）
- 双击某行 → `emit buyRequested(productId)`（**不要在这里建订单**，也别忘了拦自己的商品：`if (p.sellerId == Session::instance().currentUser().id) { QMessageBox::information(this,"提示","这是你自己发布的商品"); return; }`）

⚠️ 坑：`QTableWidget` 每行都 `new QTableWidgetItem`，刷新前必须 `table->setRowCount(0)` 清空，否则越刷越多。

✅ **验证**：切"图书教材"只剩教材类；输"iPhone"能搜到；价格 100-500 只剩中间那几件；换排序顺序真的变了。

### 11. 发布对话框

`QDialog`，`QFormLayout` 里放：标题 `QLineEdit`、分类 `QComboBox`（父分类用 `QComboBox::insertSeparator` 或加缩进前缀分组，别做树）、价格 `QDoubleSpinBox`（`setRange(0.01, 999999)`）、成色 `QSpinBox(1,10)`、描述 `QPlainTextEdit`。

确定按钮里：

```cpp
Product p;
p.title = ui->titleEdit->text();
p.categoryId = ui->catCombo->currentData().toInt();   // 用 currentData，别用 currentText
p.price = ui->priceSpin->value();
p.conditionLevel = ui->condSpin->value();
p.description = ui->descEdit->toPlainText();
auto r = m_service->publish(p, Session::instance().currentUser().id);
if (!r.ok) { QMessageBox::warning(this, "发布失败", r.message); return; }
accept();
```

✅ **验证**：发一件 → 检索页立刻能搜到（靠 `productsChanged` 信号）；价格填 0 会被拦住。

### 12. 我的商品页

`QTableWidget` + 最后一列放按钮。按状态决定按钮：

```cpp
// ON_SALE  → 「下架」+「修改」
// OFF_SALE → 「上架」+「修改」
// SOLD     → 一个都不显示（已成交的东西不许改、不许动）
```

⚠️ 坑：按钮要在 `setCellWidget` 里建，并且用 `lambda` 捕获 `productId` 时**按值捕获**（`[this, id]`），否则循环里全拿到最后一个 id。

**「修改」怎么做**（最省事的做法）：**把发布对话框复用成编辑对话框**，加一个 `void setProduct(const Product& p)`：

```cpp
// publish_dialog.cpp
void PublishDialog::setProduct(const Product& p)
{
    m_editing  = p.id;                      // 有 id = 编辑模式，0 = 新建
    m_sellerId = p.sellerId;
    ui->titleEdit->setText(p.title);
    ui->catCombo->setCurrentIndex(ui->catCombo->findData(p.categoryId));
    ui->priceSpin->setValue(p.price);
    ui->condSpin->setValue(p.conditionLevel);
    ui->descEdit->setPlainText(p.description);
    setWindowTitle(QStringLiteral("修改商品"));
}

// 确定按钮里分两路
Product p;
p.id = m_editing;
/* 其余字段照常从界面取 */
OpResult r = m_editing > 0
           ? m_service->updateInfo(p, Session::instance().currentUser().id)
           : m_service->publish(p, Session::instance().currentUser().id);
if (!r.ok) { QMessageBox::warning(this, "保存失败", r.message); return; }
accept();
```

> 一个对话框、两行分支，换来"上架"和"修改"两个功能——这是本项目性价比最高的一处复用，答辩时可以讲。

✅ **验证**：下架后检索页看不到、我的商品里显示"已下架"、重新上架又出现；改商品价格 → 检索页价格跟着变；已售出的那件没有按钮也没有"修改"。

### 13. 发信号给 B

检索页只需要一个信号：

```cpp
signals:
    void buyRequested(int productId);
```

由 D 在 `mainwindow.cpp` 里连到 B 的确认框。**你不要直接调 B 的任何东西。**

---

## 第 3 周

- 和 B 联调：点购买 → 下单成功 → 回检索页那件商品消失（变成 SOLD）
- 和 C 联调：取消订单 → 商品回到在售，能再被买
- 修自己页面上的问题

---

## 验收清单

- [ ] 发布一件商品，检索页立刻能搜到，重启程序还在
- [ ] 关键字 / 分类 / 价格区间 / 三种排序都对
- [ ] 已下架、已售出的商品搜不到
- [ ] 只能下架、只能修改自己的商品；别人的商品点不到按钮
- [ ] 改商品价格后，检索页显示的价格跟着变
- [ ] 已售出的商品既不能改也不能下架（界面上没按钮，SQL 里也挡住了）
- [ ] 自己的商品双击时提示"这是你自己发布的商品"
- [ ] 连点两次购买同一件商品，第二次被拦住

---

## 答辩时你要说的三句话

1. "检索的 WHERE 只拼结构、值全走 `bindValue`，排序字段走白名单，所以没有注入问题。"
2. "`updateStatus` 是条件更新 `WHERE id=? AND status=?`，并且检查影响行数，所以同一件商品不可能被卖两次——**拦人的是数据库，不是界面**。"
3. "发布和检索是同一个模型，列表用 `QTableWidget` 直接显示，没做分页——数据量就几十条，简单比花哨可靠。"

---

## 卡住了找谁

| 问题 | 找谁 |
|---|---|
| 表字段记不住 | 契约 §4 |
| 商品状态改不动 / 影响行数总是 0 | 先确认 `from` 传的是**数据库里的当前状态** |
| 页面挂不上主窗口 | D |
| 下单时提示"商品已售出"但明明在售 | B（他传的 productId 或 from 状态不对） |
