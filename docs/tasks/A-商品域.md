# A 商品域 —— 实现规格

这份文档是 A 这部分代码的唯一依据。**接口签名、SQL、信号名、类名一个字都不要改**，改了就接不上。给人看也给 AI 看，照抄即可。

---

## 1. 负责什么

商品的「上架 / 改信息 / 下架 / 重新上架」四件事 + 商品检索（关键字、分类、价格、排序）+ 「我的商品」列表。

**不负责**：下单（B）、订单状态流转（C）、主窗口组装（D）、数据库连接（D）。

三个界面 + 一个对话框：

| 类 | 文件 | 干什么 |
|---|---|---|
| `SearchPage` | `ui/pages/product/search_page.*` | 商品检索页（主窗口第 1 页） |
| `MyProductsPage` | `ui/pages/product/my_products_page.*` | 我的商品页（第 2 页） |
| `PublishDialog` | `ui/pages/product/publish_dialog.*` | 发布商品 / 修改商品（发布和修改复用同一个框） |

---

## 2. 文件清单

只允许动这些文件，其它文件不改（`ui/mainwindow.*`、`core/session.h`、`data/db_manager.*` 是 D 的）。

| 路径 | 现状 | 要做的 |
|---|---|---|
| `core/entity/product.h` | 已有 | **不改**，按下面第 3 节写的字段为准 |
| `core/entity/category.h` | **缺** | 新建，放 `struct Category` |
| `core/repository/i_product_repository.h` | 已有，少一个方法 | 加 `categories()` |
| `data/product_dao.h` / `.cpp` | 已有 | 加 `categories()`；`search()` 按第 4 节修两处 |
| `services/product_service.h` / `.cpp` | 已有 | 加 `categories()` |
| `ui/pages/product/product_ui.h` | 已有，有硬编码 | 删掉 `allCategories()`，只留 `conditionText()` |
| `ui/pages/product/search_page.*` | 已有 | 分类改成从 service 取；关键字匹配 description |
| `ui/pages/product/publish_dialog.*` | 已有 | 分类下拉改成从 service 取 |
| `ui/pages/product/my_products_page.*` | 已有 | 不改逻辑，检查按钮按状态显示 |

> 现在 `product_ui.h` 里那份 `allCategories()`（8 个分类写死在代码里）是临时实现，**必须删掉**，分类一律从 `categories` 表读。表里已经有约定的 8 条，见第 4 节。

---

## 3. 数据结构与接口（逐字照抄）

### 3.1 `core/entity/category.h`（新建）

```cpp
#pragma once

#include <QString>

// 分类实体：对应 categories 表的一行。parentId == 0 表示顶级分类。
struct Category
{
    int     id = 0;
    int     parentId = 0;
    QString name;
};
```

### 3.2 `core/entity/product.h`（已存在，不改）

```cpp
struct Product {
    int id=0, sellerId=0, categoryId=0;
    QString title, description, status, createdAt, updatedAt;
    double price=0.0;
    int conditionLevel=9;
    QString sellerName, categoryName;   // JOIN 出来给界面显示，不存库
};

struct ProductQuery {
    QString keyword;
    QVector<int> categoryIds;           // 空 = 不限；父类要自动带上子类
    double priceMin=-1, priceMax=-1;    // < 0 = 不限
    QString sortBy = "time_desc";       // time_desc / price_asc / price_desc
};
```

### 3.3 `core/repository/i_product_repository.h`（加一个方法）

```cpp
class IProductRepository {
public:
    virtual ~IProductRepository() = default;
    virtual OpResult insert(Product& p) = 0;
    virtual OpResult updateStatus(int id, const QString& from, const QString& to) = 0;
    virtual OpResult updateInfo(const Product& p) = 0;
    virtual std::optional<Product> findById(int id) = 0;
    virtual QVector<Product> search(const ProductQuery& q) = 0;
    virtual QVector<Product> findBySeller(int sellerId) = 0;
    virtual QVector<Category> categories() = 0;      // 新增
};
```

头文件里加 `#include "core/entity/category.h"`。

### 3.4 `services/product_service.h`（加一个方法）

```cpp
class ProductService : public QObject
{
    Q_OBJECT
public:
    explicit ProductService(IProductRepository* repo, QObject* parent = nullptr);

    OpResult publish(Product& p, int sellerId);
    OpResult updateInfo(Product p, int sellerId);
    OpResult takeDown(int productId, int sellerId);
    OpResult putOnSale(int productId, int sellerId);

    QVector<Product> search(const ProductQuery& q);
    QVector<Product> myProducts(int sellerId);
    std::optional<Product> find(int productId);
    QVector<Category> categories();                  // 新增

signals:
    void productsChanged();

private:
    IProductRepository* m_repo = nullptr;
};
```

`categories()` 直接 `return m_repo->categories();`。

---

## 4. DAO 的 SQL（照抄，绑定顺序不能乱）

`ProductDao` 里加一个常量做列清单，所有查询共用：

```cpp
namespace {
const char* SQL_FIELDS =
    "SELECT p.*, u.nickname AS seller_name, c.name AS category_name "
    "FROM products p "
    "LEFT JOIN users u ON u.id = p.seller_id "
    "LEFT JOIN categories c ON c.id = p.category_id ";
}
```

### 4.1 `insert`

```cpp
q.prepare("INSERT INTO products(seller_id, category_id, title, description, price, "
          "condition_level, status) VALUES(?,?,?,?,?,?,?)");
q.addBindValue(p.sellerId);
q.addBindValue(p.categoryId);
q.addBindValue(p.title);
q.addBindValue(p.description);
q.addBindValue(p.price);
q.addBindValue(p.conditionLevel);
q.addBindValue(p.status.isEmpty() ? Const::ProductStatus::OnSale : p.status);
```

失败返回 `OpResult::fail(QString("商品没存进去：") + q.lastError().text());`
成功时 `p.id = q.lastInsertId().toInt();` 再 `return OpResult::success(QString::number(p.id));`

### 4.2 `updateStatus`（**条件更新，这是全项目的并发保护点**）

```cpp
q.prepare("UPDATE products SET status=?, updated_at=datetime('now','localtime') "
          "WHERE id=? AND status=?");
q.addBindValue(to);      // 第 1 个 ?：目标状态
q.addBindValue(id);      // 第 2 个 ?：商品 id
q.addBindValue(from);    // 第 3 个 ?：要求当前还是这个状态
```

`q.exec()` 之后必须检查：

```cpp
if (q.numRowsAffected() != 1)
    return OpResult::fail("状态已经变了，刷新一下再试");
```

五个场景都靠这条 SQL（方法同一个，参数不同）：

| 场景 | from | to |
|---|---|---|
| 上架（新增商品） | — | `ON_SALE`（由 insert 的默认值完成） |
| 下架 | `ON_SALE` | `OFF_SALE` |
| 重新上架 | `OFF_SALE` | `ON_SALE` |
| 被买走（B 的 `createOrder` 调） | `ON_SALE` | `SOLD` |
| 取消订单放回（C 的 `changeStatus` 调） | `SOLD` | `ON_SALE` |

影响行数为 0 说明「当前状态已经不是 from」，直接失败返回，**不要**改成无条件 `UPDATE`。

### 4.3 `updateInfo`（改信息，不改状态）

```cpp
q.prepare("UPDATE products SET title=?, category_id=?, description=?, price=?, "
          "condition_level=?, updated_at=datetime('now','localtime') "
          "WHERE id=? AND status <> ?");
q.addBindValue(p.title);
q.addBindValue(p.categoryId);
q.addBindValue(p.description);
q.addBindValue(p.price);
q.addBindValue(p.conditionLevel);
q.addBindValue(p.id);
q.addBindValue(Const::ProductStatus::Sold);
```

这条**不检查** `numRowsAffected()`：用户只改了个空格时 SQLite 返回值不稳定，靠 Service 层先读一遍校验权限和状态就够了。

### 4.4 `findById`

```cpp
q.prepare(QString::fromLatin1(SQL_FIELDS) + "WHERE p.id = ?");
```

### 4.5 `search`（动态 WHERE 只拼结构，值全部绑定）

```cpp
QString sql = QString::fromLatin1(SQL_FIELDS) + "WHERE p.status = ?";
QVariantList vals;
vals << Const::ProductStatus::OnSale;          // 检索页永远只看在售

if (!q.keyword.isEmpty()) {
    sql += " AND (p.title LIKE ? OR p.description LIKE ?)";
    vals << ("%" + q.keyword + "%") << ("%" + q.keyword + "%");
}

if (!q.categoryIds.isEmpty()) {                // 几个分类就几个 ?
    sql += " AND p.category_id IN (";
    for (int i = 0; i < q.categoryIds.size(); i++) {
        if (i > 0) sql += ",";
        sql += "?";
    }
    sql += ")";
    for (int id : q.categoryIds) vals << id;
}

if (q.priceMin >= 0) { sql += " AND p.price >= ?"; vals << q.priceMin; }
if (q.priceMax >= 0) { sql += " AND p.price <= ?"; vals << q.priceMax; }

if (q.sortBy == "price_asc")       sql += " ORDER BY p.price ASC";
else if (q.sortBy == "price_desc") sql += " ORDER BY p.price DESC";
else                               sql += " ORDER BY p.created_at DESC";
```

排序**只允许**这三个分支，`sortBy` 不能拼进 SQL（防注入）。

### 4.6 `findBySeller`

```cpp
q.prepare(QString::fromLatin1(SQL_FIELDS)
          + "WHERE p.seller_id = ? ORDER BY p.created_at DESC");
q.addBindValue(sellerId);
```

### 4.7 `categories`（新增）

```cpp
q.prepare("SELECT id, name, IFNULL(parent_id, 0) AS parent_id FROM categories "
          "ORDER BY CASE WHEN parent_id IS NULL THEN 0 ELSE 1 END, id");
```

排序保证父类在前、子类在后，界面按这个顺序建树。

### 4.8 `mapRow`（列名取值，不用列序号）

```cpp
Product p;
p.id             = q.value("id").toInt();
p.sellerId       = q.value("seller_id").toInt();
p.categoryId     = q.value("category_id").toInt();
p.title          = q.value("title").toString();
p.description    = q.value("description").toString();
p.price          = q.value("price").toDouble();
p.conditionLevel = q.value("condition_level").toInt();
p.status         = q.value("status").toString();
p.createdAt      = q.value("created_at").toString();
p.updatedAt      = q.value("updated_at").toString();
p.sellerName     = q.value("seller_name").toString();
p.categoryName   = q.value("category_name").toString();
```

---

## 5. Service 层规则

`publish` 和 `updateInfo` 的字段校验完全一样，顺序固定：

```cpp
p.title = p.title.trimmed();
if (p.title.isEmpty())                 return OpResult::fail("标题还没写");
if (p.title.size() > 60)               return OpResult::fail("标题太长了，60 个字以内");
if (p.price <= 0)                      return OpResult::fail("价格得大于 0");
if (p.categoryId <= 0)                 return OpResult::fail("还没选分类");
if (p.conditionLevel < 1 || p.conditionLevel > 10)
                                       return OpResult::fail("成色只能填 1 到 10");
```

| 方法 | 额外规则 |
|---|---|
| `publish` | `p.sellerId = sellerId; p.status = Const::ProductStatus::OnSale;` 再 insert |
| `updateInfo` | 先 `findById`：不存在 → 「找不到这个商品」；`old->sellerId != sellerId` → 「这不是你发布的商品」；`old->status == Sold` → 「已经卖掉了，改不了」 |
| `takeDown` | 不存在 → 同上；不是自己的 → 「只能下架自己的商品」；已售出 → 「已经卖掉了，下不了架」；然后 `updateStatus(id, ON_SALE, OFF_SALE)` |
| `putOnSale` | 同上，但 `updateStatus(id, OFF_SALE, ON_SALE)`；文案「只能上架自己的商品」「已经卖掉了，上不了架」 |

写操作成功后一律 `emit productsChanged();`，失败不 emit。

**`sellerId` 谁传**：页面从 `Session::instance().currentUser().id` 取，Service 不自己读 Session。

---

## 6. 界面

三个类的成员、构造签名、信号固定如下，D 的 `MainWindow` 就是按这个 new 的。

### 6.1 `SearchPage`（检索页）

```cpp
class SearchPage : public QWidget
{
    Q_OBJECT
public:
    explicit SearchPage(ProductService* service, QWidget* parent = nullptr);
    void refresh();                       // MainWindow::onDataChanged 会调

signals:
    void buyRequested(int productId);     // 双击一行 → D 去开购买确认框
    void publishRequested();              // 点「发布商品」→ D 去开 PublishDialog

private slots:
    void onRowDoubleClicked(int row, int column);

private:
    QVector<int> selectedIds() const;     // 勾选的分类；选父类自动带子类 id
};
```

界面元素：

| 控件 | 类型 | 说明 |
|---|---|---|
| `m_catList` | `QListWidget` | 分类列表，条目文字：顶级就是名字，子级显示 `父 / 子`；`item->setData(Qt::UserRole, id)` 存 id |
| `m_kw` | `QLineEdit` | 关键字，placeholder 写「标题或描述」 |
| `m_min` / `m_max` | `QDoubleSpinBox` | 价格区间，`range(0, 999999)`，0 表示不限 |
| `m_sort` | `QComboBox` | 三项，`itemData` 分别是 `time_desc` / `price_asc` / `price_desc` |
| `m_table` | `QTableWidget` | 4 列：标题 / 分类 / 价格 / 卖家；`setEditTriggers(NoEditTriggers)`、`setSelectionBehavior(SelectRows)`、整行双击 |
| `m_count` | `QLabel` | 「共 N 件」 |

行为：

- 构造时 `m_cats = service->categories();` 用它建 `m_catList`（子级文字取父名拼 `父 / 子`，父名从同一份列表里查）。**数据必须来自 service，不许写死。**
- `refresh()`：组 `ProductQuery`（keyword = `m_kw->text().trimmed()`、categoryIds = `selectedIds()`、价格、sortBy = `m_sort->currentData().toString()`），调 `m_service->search(q)` 填表；每行 `Qt::UserRole` 存 `product.id`；更新 `m_count`。
- `selectedIds()`：遍历选中的分类，`add(id)`；如果某个选中的是父类（`parentId == 0`），把它所有子类 id 一起加进去。
- 双击行：取该行 `Qt::UserRole` → `emit buyRequested(id);`。**检索页不直接开购买框、不调 OrderService。**
- 点「发布商品」按钮 → `emit publishRequested();`。

### 6.2 `MyProductsPage`（我的商品）

```cpp
class MyProductsPage : public QWidget
{
    Q_OBJECT
public:
    explicit MyProductsPage(ProductService* service, QWidget* parent = nullptr);
    void refresh();
};
```

- `refresh()`：`m_service->myProducts(Session::instance().currentUser().id)` 填表，列：标题 / 分类 / 价格 / 成色（`conditionText`）/ 状态（`Const::zh(status)`）/ 操作。
- 末列放两个 `QPushButton`（`setCellWidget`）：`编辑` 与 `下架`/`上架`。按状态决定显示：
  - `ON_SALE` → 编辑 + 下架
  - `OFF_SALE` → 编辑 + 上架
  - `SOLD` → 两个按钮都 `setEnabled(false)`
- 编辑 → `PublishDialog* dlg = new PublishDialog(m_service, uid, this); dlg->setProduct(p); dlg->exec(); delete dlg;`（或用 `QDialog::exec()` 前设 `Qt::WA_DeleteOnClose`，二选一，别两样都写）
- 下架 → `m_service->takeDown(id, uid)`；上架 → `m_service->putOnSale(id, uid)`。失败 `QMessageBox::warning(this, "失败", r.message)`；成功不用弹（`productsChanged` 会触发刷新）。

### 6.3 `PublishDialog`（发布 / 修改共用一个框）

```cpp
class PublishDialog : public QDialog
{
    Q_OBJECT
public:
    PublishDialog(ProductService* service, int sellerId, QWidget* parent = nullptr);
    void setProduct(const Product& product);   // 传了就是「修改」模式

private slots:
    void onSubmitClicked();
};
```

- `m_editId` 初值 0；`setProduct()` 里 `m_editId = product.id`，回填标题/分类/价格/成色/描述，窗口标题改「修改商品」，按钮文字改「保存」。
- 分类下拉：`for (const Category& c : m_service->categories())`，文字子级用 `父 / 子`，`itemData` 存 `id`。
- `onSubmitClicked()`：
  - 组 `Product p`，`p.id = m_editId`，填 `title`/`categoryId`（`m_cat->currentData().toInt()`）/`price`/`conditionLevel`/`description`；
  - `m_editId > 0` 调 `m_service->updateInfo(p, m_sellerId)`，否则 `m_service->publish(p, m_sellerId)`；
  - 失败：`m_err->setText(r.message)` 显示红字，**不弹窗、不关窗口**；
  - 成功：`accept()`。

### 6.4 `product_ui.h`（只留一个函数）

```cpp
#pragma once
#include <QString>

// 成色数字 → 界面显示文字。分类不在这里，分类从 categories 表读。
inline QString conditionText(int lv)
{
    if (lv >= 10)
        return "全新";
    return QString::number(lv) + "成新";
}
```

---

## 7. 与别人的接缝

### 7.1 你要用的（别人提供，别重复实现）

| 东西 | 位置 | 怎么用 |
|---|---|---|
| `Const::ProductStatus::{OnSale, OffSale, Sold}` | `core/common/constants.h` | 状态字面量只用这三个常量 |
| `Const::zh(code)` | 同上 | 状态码转中文，别自己写 if/else |
| `OpResult` | `core/common/opresult.h` | `OpResult::success()` / `OpResult::fail("原因")`；`r.ok`、`r.message` |
| `Session::instance()` | `core/session.h` | `currentUser()` / `isLoggedIn()` |
| `DbManager::db()` | `data/db_manager.h` | DAO 里取连接，别自己 `addDatabase` |

### 7.2 D 的 `MainWindow` 会这样用你

```cpp
m_searchPage     = new SearchPage(m_productService, this);
m_myProductsPage = new MyProductsPage(m_productService, this);

connect(m_searchPage, &SearchPage::buyRequested,     this, &MainWindow::onBuyRequested);
connect(m_searchPage, &SearchPage::publishRequested, this, &MainWindow::onPublishRequested);
connect(m_productService, &ProductService::productsChanged, this, &MainWindow::onDataChanged);
```

`onDataChanged()` 里会调 `m_searchPage->refresh()`、`m_myProductsPage->refresh()`。
所以：**这两个页面必须有公开的无参 `refresh()`**，构造函数第二参数必须是 `QWidget* parent`。

### 7.3 谁依赖你

- B 的 `OrderService::createOrder` 调 `IProductRepository::updateStatus(productId, ON_SALE, SOLD)`——你的条件更新成功与否直接决定下单成败。
- C 的 `TradeService::changeStatus` 在取消订单时调 `updateStatus(productId, SOLD, ON_SALE)`。
- D 的演示数据直接 SQL 插 12 件商品，`status` 全是 `'ON_SALE'`。

---

## 8. 硬性禁止

1. 不在 `ui/` 里写任何 SQL。
2. 不自己 `new` `ProductDao`——Service 由 D 在 `buildServices()` 里 new 好传进来。
3. 不改 `core/entity/product.h` 的字段（B、C、D 都在用）。
4. 不把分类写回代码常量，必须查表。
5. 动态 SQL 只拼结构，值一律 `addBindValue`。
6. 状态字面量不写字符串，用 `Const::ProductStatus::*`。
7. 不在别的目录新建 `.cpp`（只允许 `core/`、`data/`、`services/`、`ui/pages/product/`）。

---

## 9. 验收清单

- [ ] 新增/修改文件后执行 `扩展 → Qt VS Tools → Open Qt Project File (.pro)`，再 `Ctrl+Shift+B`，0 错误。
- [ ] 检索页能列出 12 件在售商品；关键字能搜到「描述里含该词」的商品（不只看标题）。
- [ ] 只勾「数码电子」，能搜出挂在它下面的手机 / 电脑 / 耳机。
- [ ] 价格区间 100~500 只出这个区间的；三种排序都试一遍。
- [ ] 发布一件商品后，检索页立刻出现（`productsChanged` 生效）。
- [ ] 「我的商品」里：在售能下架、下架能上架、已售出两个按钮都点不动。
- [ ] 修改标题后检索页标题跟着变；已售出的点「编辑」提示「已经卖掉了，改不了」。
- [ ] 两个账号交替登录，各自「我的商品」互不可见（`findBySeller` 用当前用户 id）。

---

## 10. 已知坑

| 现象 | 原因 | 处理 |
|---|---|---|
| `LNK2019 无法解析的外部符号 ProductDao::...` | 新加了 `.cpp` 但没进工程 | 重新 `Open Qt Project File (.pro)` 再编 |
| `C4828` / `C2001 常量中有换行符` | 文件存成了 GBK | 仓库根有 `.editorconfig`；老文件用 `文件 → 高级保存选项` 存 UTF-8 无签名 |
| 下架后再下架没反应也不报错 | `updateStatus` 没检查 `numRowsAffected()` | 按 4.2 加判断并 `fail("状态已经变了，刷新一下再试")` |
| 检索页分类写死 8 条 | 还在用 `allCategories()` | 删掉它，改 `m_service->categories()` |
| 双击行弹出两个购买框 | 双击信号连了两次 | 只连 `cellDoubleClicked` 一次 |
| 修改商品后列表价格没变 | `updateInfo` 失败但界面吞了返回值 | 每个写操作都要看 `r.ok`，失败必须提示 |
