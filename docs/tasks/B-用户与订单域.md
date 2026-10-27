# B 用户与订单域 —— 实现规格

这份文档是 B 这部分代码的唯一依据。**接口签名、SQL、信号名、类名一个字都不要改**，改了就接不上。给人看也给 AI 看，照抄即可。

---

## 1. 负责什么

- 用户：注册（即寄售登记）、登录、退出、登录态（`Session`）。
- 订单：下单（跨三个域的事务）、订单记录查询（我买到的 / 我卖出的）、订单详情（含状态操作按钮）。

**不负责**：商品增删改查（A）、状态机与状态流转的规则本身（C 的 `TradeService`/`TradeStateMachine`）、主窗口组装（D）。

界面/服务清单：

| 类 | 文件 | 现状 |
|---|---|---|
| `LoginDialog` | `ui/pages/auth/login_dialog.*` | 已有（登录/注册两态复用） |
| `BuyDialog` | `ui/pages/order/buy_dialog.*` | **缺**，要写 |
| `MyOrdersPage` | `ui/pages/order/my_orders_page.*` | **缺**，要写（买到的 / 卖出的共用一个类） |
| `OrderDetailDialog` | `ui/pages/order/order_detail_dialog.*` | **缺**，要写 |

---

## 2. 文件清单

| 路径 | 现状 | 要做的 |
|---|---|---|
| `core/entity/user.h` | 已有 | 不改 |
| `core/entity/order.h` | 已有 | 不改 |
| `core/session.h` | 已有 | 不改（D 的） |
| `core/repository/i_user_repository.h` | 已有 | 不改 |
| `core/repository/i_order_repository.h` | 已有 | 不改 |
| `data/user_dao.h/.cpp` | 已有 | 不改 |
| `data/order_dao.h/.cpp` | 已有 | 不改 |
| `services/auth_service.h/.cpp` | 已有 | 不改 |
| `services/order_service.h/.cpp` | 已有 | 加一个 `findOrder()` |
| `ui/pages/auth/login_dialog.*` | 已有 | 不改 |
| `ui/pages/order/buy_dialog.*` | **缺** | 新建 |
| `ui/pages/order/my_orders_page.*` | **缺** | 新建 |
| `ui/pages/order/order_detail_dialog.*` | **缺** | 新建 |

---

## 3. 数据结构与接口（逐字照抄）

### 3.1 实体

```cpp
struct User
{
    int     id = 0;
    QString username;
    QString nickname;
    QString passwordHash;   // DAO::insert 收到的这个字段是【明文】，DAO 内部哈希后再存
    QString salt;
    QString createdAt;
};

struct Order
{
    int     id = 0;
    int     productId = 0;
    int     buyerId = 0;
    int     sellerId = 0;
    QString orderNo;
    QString status;
    QString remark;
    QString createdAt;
    QString updatedAt;
    double  amount = 0.0;
    QString productTitle;   // 以下三个 JOIN 出来，不存库
    QString buyerName;
    QString sellerName;
};

struct OrderQuery
{
    int     buyerId = 0;    // 二选一，0 表示不限
    int     sellerId = 0;
    QString status;
    QString keyword;        // 匹配 order_no 或 product_title
};
```

### 3.2 仓储接口（不改）

```cpp
class IUserRepository
{
public:
    virtual ~IUserRepository() = default;
    virtual OpResult insert(User& u) = 0;                      // 传明文密码，成功回填 u.id
    virtual std::optional<User> findByUsername(const QString& name) = 0;
    virtual std::optional<User> findById(int id) = 0;
    virtual bool exists(const QString& name) = 0;
};

class IOrderRepository
{
public:
    virtual ~IOrderRepository() = default;
    virtual OpResult insert(Order& o) = 0;                     // 成功回填 o.id
    virtual OpResult updateStatus(int orderId, const QString& from, const QString& to) = 0;
    virtual std::optional<Order> findById(int id) = 0;
    virtual QVector<Order> search(const OrderQuery& q) = 0;
    virtual QString nextOrderNo() = 0;                         // 形如 20261025-0001
};
```

### 3.3 `services/order_service.h`（加一个方法）

```cpp
class OrderService : public QObject
{
    Q_OBJECT
public:
    OrderService(IOrderRepository* orderRepo, IProductRepository* productRepo,
                 ITradeLogRepository* logRepo, QObject* parent = nullptr);

    // 跨三个域的事务：建订单 + 商品置已售 + 写首条日志，任一步失败整体回滚
    OpResult createOrder(int productId, int buyerId, const QString& remark, int* outOrderId);

    QVector<Order> queryOrders(const OrderQuery& q);

    std::optional<Order> findOrder(int orderId);   // 新增：订单详情页用

signals:
    void orderCreated(int orderId);
};
```

`findOrder` 就一行：`return m_orderRepo->findById(orderId);`

---

## 4. DAO 的 SQL（照抄）

### 4.1 `UserDao::insert`（明文进、哈希出）

```cpp
if (u.username.isEmpty())  return OpResult::fail("用户名不能为空");
if (u.passwordHash.isEmpty()) return OpResult::fail("密码不能为空");

if (u.salt.isEmpty())
    u.salt = QUuid::createUuid().toString(QUuid::Id128);

const QString hash = sha256Hex(u.salt + u.passwordHash);   // 明文 + 盐，再 sha256

q.prepare("INSERT INTO users(username, password_hash, salt, nickname) VALUES(?, ?, ?, ?)");
q.addBindValue(u.username);
q.addBindValue(hash);
q.addBindValue(u.salt);
q.addBindValue(u.nickname.isEmpty() ? u.username : u.nickname);
```

- `sha256Hex` 是文件内匿名命名空间里的静态函数：`QCryptographicHash::hash(s.toUtf8(), QCryptographicHash::Sha256).toHex()`。
- 失败时若 `q.lastError().nativeErrorCode() == "19"`（UNIQUE 冲突）返回 `OpResult::fail("用户名已被占用")`。
- 成功后必须写回：`u.id = q.lastInsertId().toInt(); u.passwordHash = hash;`

### 4.2 `UserDao::verifyPassword`（静态，登录用）

```cpp
if (u.salt.isEmpty() || u.passwordHash.isEmpty()) return false;
return sha256Hex(u.salt + plain) == u.passwordHash;
```

### 4.3 订单查询的公共列

```cpp
const char* kSelect =
    "SELECT o.id, o.order_no, o.product_id, o.buyer_id, o.seller_id, o.amount, o.status, "
    "       o.remark, o.created_at, o.updated_at, "
    "       p.title AS product_title, b.nickname AS buyer_name, s.nickname AS seller_name "
    "FROM orders o "
    "LEFT JOIN products p ON p.id = o.product_id "
    "LEFT JOIN users b ON b.id = o.buyer_id "
    "LEFT JOIN users s ON s.id = o.seller_id";
```

### 4.4 `OrderDao::nextOrderNo`

```cpp
const QString day = QDate::currentDate().toString("yyyyMMdd");
q.prepare("SELECT COUNT(*) FROM orders WHERE order_no LIKE ?");
q.addBindValue(day + "-%");
q.exec(); q.next();
const int n = q.value(0).toInt() + 1;
return day + "-" + QString("%1").arg(n, 4, 10, QChar('0'));
```

### 4.5 `OrderDao::insert`

```cpp
q.prepare("INSERT INTO orders(order_no, product_id, buyer_id, seller_id, amount, status, remark) "
          "VALUES(?, ?, ?, ?, ?, ?, ?)");
q.addBindValue(o.orderNo);
q.addBindValue(o.productId);
q.addBindValue(o.buyerId);
q.addBindValue(o.sellerId);
q.addBindValue(o.amount);
q.addBindValue(Const::OrderStatus::PendingPay);   // 新订单一律待付款
q.addBindValue(o.remark);
```

成功后 `o.id = q.lastInsertId().toInt(); o.status = Const::OrderStatus::PendingPay;`

### 4.6 `OrderDao::updateStatus`（条件更新）

```cpp
q.prepare("UPDATE orders SET status = ?, updated_at = datetime('now','localtime') "
          "WHERE id = ? AND status = ?");
q.addBindValue(to);
q.addBindValue(orderId);
q.addBindValue(from);
// exec 后：
if (q.numRowsAffected() != 1)
    return OpResult::fail("订单状态已变，请刷新");
```

### 4.7 `OrderDao::search`

```cpp
QString sql = QString(kSelect) + " WHERE 1 = 1";
QVariantList binds;

if (cond.buyerId > 0)  { sql += " AND o.buyer_id = ?";  binds << cond.buyerId; }
if (cond.sellerId > 0) { sql += " AND o.seller_id = ?"; binds << cond.sellerId; }
if (!cond.status.isEmpty()) { sql += " AND o.status = ?"; binds << cond.status; }
if (!cond.keyword.isEmpty()) {
    sql += " AND (o.order_no LIKE ? OR p.title LIKE ?)";
    const QString like = "%" + cond.keyword + "%";
    binds << like << like;
}
sql += " ORDER BY o.created_at DESC, o.id DESC";
```

`buyerId` 和 `sellerId` 同时 >0 时是「与」关系（一般不会这么传）。

---

## 5. Service 层规则

### 5.1 `AuthService::login`

```cpp
if (username.trimmed().isEmpty()) return OpResult::fail("请输入用户名");
const auto u = m_repo->findByUsername(username.trimmed());
if (!u || !UserDao::verifyPassword(*u, password))
    return OpResult::fail("用户名或密码错误");   // 不说是哪个错
Session::instance().setCurrentUser(*u);
emit loginSucceeded();
return OpResult::success();
```

### 5.2 `AuthService::registerUser(username, nickname, pwd, pwd2)`

| 检查 | 失败文案 |
|---|---|
| `username.trimmed().size() < 3` | 「用户名至少 3 个字符」 |
| `pwd.size() < 6` | 「密码至少 6 位」 |
| `pwd != pwd2` | 「两次密码不一致」 |
| `m_repo->exists(username.trimmed())` | 「用户名已被占用」 |

通过后组 `User u`：`u.username = username.trimmed()`、`u.nickname = nickname.trimmed().isEmpty() ? u.username : nickname.trimmed()`、`u.passwordHash = pwd`（**明文**交给 DAO）。`insert` 成功后 `Session::instance().setCurrentUser(u); emit loginSucceeded();`

### 5.3 `AuthService::logout`

`Session::instance().clear(); emit logoutDone();`

### 5.4 `OrderService::createOrder`（**本项目最重要的一段**）

事务外先做三个前置检查（挡在事务外面，省得白开事务）：

```cpp
const auto p = m_productRepo->findById(productId);
if (!p)                                        return OpResult::fail("商品不存在");
if (p->status != Const::ProductStatus::OnSale)  return OpResult::fail("该商品已售出或已下架");
if (p->sellerId == buyerId)                    return OpResult::fail("不能购买自己发布的商品");
```

然后一个事务里三步，顺序不能换：

```cpp
QSqlDatabase& db = DbManager::db();
if (!db.transaction()) return OpResult::fail("开启事务失败");

// ① 建订单
Order o;
o.productId = productId;
o.buyerId   = buyerId;
o.sellerId  = p->sellerId;
o.amount    = p->price;          // 金额取商品当前价，不取界面传来的
o.remark    = remark;
o.orderNo   = m_orderRepo->nextOrderNo();
OpResult r = m_orderRepo->insert(o);
if (!r.ok) { db.rollback(); return r; }

// ② 商品置已售（A 的条件更新：还在售才改得动，连点两次只有一次成功）
r = m_productRepo->updateStatus(productId, Const::ProductStatus::OnSale,
                                Const::ProductStatus::Sold);
if (!r.ok) { db.rollback(); return OpResult::fail("该商品刚刚被买走了，请刷新"); }

// ③ 写第一条状态日志（C 的表）
TradeLog log;
log.orderId    = o.id;
log.fromStatus = QString();                    // 下单之前没有状态，空串
log.toStatus   = Const::OrderStatus::PendingPay;
log.action     = Const::TradeAction::Create;
log.operatorId = buyerId;
log.remark     = QStringLiteral("下单");
r = m_logRepo->insert(log);
if (!r.ok) { db.rollback(); return r; }

if (!db.commit()) { db.rollback(); return OpResult::fail("提交事务失败"); }

if (outOrderId) *outOrderId = o.id;
emit orderCreated(o.id);
return OpResult::success(QString::number(o.id));
```

**注意**：DAO 一律不开事务，事务只在这里和 C 的 `changeStatus` 里开。

**两条失败提示各自在什么情况下出现**（别混，验收项写错了会误判成 bug）：

| 提示 | 在哪一行 | 什么时候真的出现 |
| --- | --- | --- |
| `该商品已售出或已下架` | 事务外的前置检查（上面前三行） | 商品在**打开购买框之前**就已经不是 `ON_SALE` 了。单机单用户下，同一商品连点两次第二次走的就是这一条（第二次进来时商品已被第一次置成 `SOLD`） |
| `该商品刚刚被买走了，请刷新` | 事务内第 ② 步条件更新失败 | 前置检查通过、但 `UPDATE ... WHERE status='ON_SALE'` 没改到行。单机同步执行时**几乎不可能**触发，它的作用是兜住"检查与更新之间被另一个连接抢先改掉"的竞态（比如两个程序实例同时开着 demo 库）。属于防御性代码，不是主路径 |

### 5.5 `OrderService::queryOrders`

`return m_orderRepo->search(q);` —— 买到的传 `buyerId`，卖出的传 `sellerId`。

---

## 6. 界面

### 6.1 `LoginDialog`（已存在，不要改签名）

```cpp
class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(AuthService* auth, QWidget* parent = nullptr);
};
```

规则：登录成功后 `accept()`（D 的 `main.cpp` 靠 `exec() == QDialog::Accepted` 判断）；失败把文案显示在 `m_errorLabel`（红字），不弹窗、不关窗口；「没有账号？去注册」按钮调 `setRegisterMode(true)`，切换后显示昵称和确认密码两行。

### 6.2 `BuyDialog`（新建）

```cpp
class BuyDialog : public QDialog
{
    Q_OBJECT
public:
    BuyDialog(OrderService* orderService, ProductService* productService,
              int productId, int buyerId, QWidget* parent = nullptr);
};
```

- 构造里 `const auto p = productService->find(productId);` 取商品；取不到直接 `QMessageBox::warning` 后 `reject()`。
- 展示：标题、分类、价格、卖家昵称、成色（`conditionText`），再加一个备注输入框（`QLineEdit`，可空）。
- 「确认购买」→ `orderService->createOrder(m_productId, m_buyerId, remark, nullptr)`：
  - 成功：`QMessageBox::information(this, "下单成功", "订单已创建，请到「我的订单」查看");` 然后 `accept()`；
  - 失败：`QMessageBox::warning(this, "下单失败", r.message);` **不关窗口**（用户可能想重试）。
- 按钮 disabled 的时机：产物已售出时不开这个框由 D 负责，这里不用额外判断。

### 6.3 `MyOrdersPage`（新建，买到的/卖出的共用一个类）

```cpp
class MyOrdersPage : public QWidget
{
    Q_OBJECT
public:
    MyOrdersPage(OrderService* orderService, int userId, bool asSeller,
                 QWidget* parent = nullptr);

    void refresh();       // MainWindow::onDataChanged 会调
    void setUserId(int userId);
};
```

- `asSeller == false` → 查 `OrderQuery{buyerId = userId}`，表头「卖家 / 商品 / 金额 / 状态 / 下单时间」；
  `asSeller == true` → 查 `OrderQuery{sellerId = userId}`，表头「买家 / 商品 / 金额 / 状态 / 下单时间」。
- 每行 `Qt::UserRole` 存 `order.id`；`setEditTriggers(NoEditTriggers)`、整行选择。
- 顶部一个状态筛选 `QComboBox`（「全部」+ 五个状态，`itemData` 存状态码），一个关键字 `QLineEdit`（placeholder「订单号或商品名」），一个「查询」按钮。
- 双击行 → 打开订单详情：`OrderDetailDialog dlg(m_orderService, m_tradeService, orderId, this); dlg.exec();` 这里需要 `TradeService*`，所以构造签名实际是：

```cpp
MyOrdersPage(OrderService* orderService, TradeService* tradeService,
             int userId, bool asSeller, QWidget* parent = nullptr);
```

- 界面里**不写任何状态判断逻辑**（哪些按钮可点由 C 的 `TradeStateMachine::actionsFor` 决定）。

### 6.4 `OrderDetailDialog`（新建）

```cpp
class OrderDetailDialog : public QDialog
{
    Q_OBJECT
public:
    OrderDetailDialog(OrderService* orderService, TradeService* tradeService,
                      int orderId, QWidget* parent = nullptr);
};
```

布局三块，从上到下：

1. **订单信息**：订单号、商品、金额、买家、卖家、当前状态（`Const::zh(status)`）、备注、创建时间。数据用 `orderService->findOrder(orderId)`。
2. **状态操作按钮**：`for (const QString& a : TradeStateMachine::actionsFor(order.status))` 建按钮，文字 `TradeStateMachine::actionName(a)`，点击调 `tradeService->changeStatus(m_orderId, a, 当前用户 id)`：
   - 成功 → `accept()`（让父页面 refresh）；
   - 失败 → `QMessageBox::warning(this, "操作失败", r.message)`（权限不够、状态已变都会走到这里）。
3. **状态日志表**：`tradeService->orderLogs(orderId)`，列：时间 / 动作 / 从 / 到 / 操作人 / 备注。**直接按返回顺序显示**（DAO 已按 `id ASC` 排好），不要自己再按时间排。

---

## 7. 与别人的接缝

### 7.1 你要用的

| 东西 | 位置 | 用途 |
|---|---|---|
| `Const::OrderStatus::*`、`Const::TradeAction::Create`、`Const::zh()` | `core/common/constants.h` | 状态/动作字面量 |
| `OpResult` | `core/common/opresult.h` | 写操作返回值 |
| `Session::instance()` | `core/session.h` | 当前用户 `id`、`nickname` |
| `IProductRepository::findById` / `updateStatus` | `core/repository/i_product_repository.h` | 下单时读商品、置已售 |
| `ITradeLogRepository::insert` | `core/repository/i_trade_log_repository.h` | 写首条日志 |
| `TradeStateMachine::actionsFor/actionName` | `services/trade_state_machine.h` | 详情页按钮 |
| `TradeService::changeStatus/orderLogs` | `services/trade_service.h` | 详情页改状态、看日志 |
| `ProductService::find` | `services/product_service.h` | 购买框显示商品信息 |
| `conditionText(int)` | `ui/pages/product/product_ui.h` | 成色文字 |

### 7.2 D 的 `MainWindow` 会这样用你

```cpp
// 登录：D 在 main.cpp 里
LoginDialog loginDlg(&authService);
if (loginDlg.exec() != QDialog::Accepted) return 0;
// MainWindow 之后
w.setUserId(Session::instance().currentUser().id);

// 页面
m_myOrdersPage   = new MyOrdersPage(m_orderService, m_tradeService, uid, false, this);  // 我买到的
m_mySalesPage    = new MyOrdersPage(m_orderService, m_tradeService, uid, true,  this);  // 我卖出的

// 买：D 的 onBuyRequested(int productId)
BuyDialog dlg(m_orderService, m_productService, productId, uid, this);
dlg.exec();

connect(m_orderService, &OrderService::orderCreated, this, &MainWindow::onDataChanged);
```

**所以这些构造函数签名一个字都不能改**，`refresh()` 必须公开无参。

### 7.3 谁依赖你

- C 的 `TradeService` 调 `IOrderRepository::findById` / `updateStatus` 改订单状态。
- D 的演示数据用到 `UserDao::insert`（三个账号）、`OrderDao` 的表结构。
- 所有人用 `Session` 取当前用户。

---

## 8. 硬性禁止

1. 不在 `ui/` 里写 SQL，不在 Service 里碰 `QWidget`。
2. 不用 `QMessageBox` 做「成功」提示以外的流程控制（失败提示可以）。
3. 不自己算订单号/金额（`nextOrderNo()`、`p->price`）。
4. 不在 DAO 里开事务。
5. 密码只在 `UserDao` 里哈希，其它地方不出现 `QCryptographicHash`。
6. 状态字面量必须用 `Const::`。
7. 不改 A 的 `Product`、C 的 `TradeLog` 结构体。

---

## 9. 验收清单

- [ ] 新增文件后 `Open Qt Project File (.pro)` 再 `Ctrl+Shift+B`，0 错误。
- [ ] 注册 `tom` / `123456` 成功并自动登录；用 `tom` 再注册提示「用户名已被占用」。
- [ ] 密码输错、用户名不存在，都提示同一句「用户名或密码错误」。
- [ ] 登录后 `/data/app.db` 的 `users` 表里 `password_hash` 是 64 位十六进制、`salt` 非空（不是明文）。
- [ ] 下单成功：`orders` 多一行（`order_no` 形如 `20261027-0001`、`status=PENDING_PAY`、`amount` 等于商品价）、`products.status` 变 `SOLD`、`trade_log` 多一行（`from_status` 为空、`action=CREATE`）。
- [ ] 对同一商品连点两次「确认购买」：第二次提示「该商品已售出或已下架」（走的是 5.4 的前置检查），且 `orders` 里只有一条记录。
- [ ] 用自己的号买自己的商品：提示「不能购买自己发布的商品」。
- [ ] 「我买到的」与「我卖出的」两个页面看到的是同一张订单的两个视角，状态一致。
- [ ] 订单详情里日志顺序就是从下到上依次「下单 → 付款 → 发货 → 确认收货」。

---

## 10. 已知坑

| 现象 | 原因 | 处理 |
|---|---|---|
| `LNK2019 无法解析的外部符号` | 新增 `.cpp` 没进工程 | 重新 `Open Qt Project File (.pro)` |
| `C2039 "toVariant" 不是 QVariant 的成员` | 抄了不存在的写法 | 用 `.toString()` / `.toInt()` |
| 下单后商品还是「在售」 | 事务里漏了第 ② 步，或漏了 `commit()` | 按 5.4 三步顺序 + commit |
| 连点两次生成两单 | 用了无条件 `UPDATE products SET status='SOLD'` | 必须走条件更新并检查 `numRowsAffected()`；前置检查拦得住第二次点击，条件更新兜住并发 |
| 订单号重复报 `UNIQUE constraint failed` | `nextOrderNo()` 在事务外算 | 必须在事务内、`insert` 之前算 |
| 订单详情页按钮点了没反应 | `actionsFor` 返回的码直接当文字用了 | 文字用 `actionName(a)`，码传给 `changeStatus` |
| 登录后页面还是空的 | 没接 `loginSucceeded` 或没调 `refresh()` | 页面必须有公开 `refresh()` |
