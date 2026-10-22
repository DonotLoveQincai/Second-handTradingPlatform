# 任务表 B · 用户与订单域（登记 / 下单 / 订单记录）

> **一句话**：谁能登录，谁买了什么，记录查得出来。
> **你不用懂**：商品怎么搜（A）、状态机怎么写的（C）、窗口怎么搭的（D）。
> **你依赖 D**：`DbManager`、`OpResult`、`Const::`、主窗口挂载点。
> **你依赖 A**：`IProductRepository::findById` / `updateStatus`。
> **别人依赖你**：C 改状态要调你的 `IOrderRepository::updateStatus`；所有人读 `Session`。

---

## 你要改的文件（只有这些）

```
core/entity/user.h
core/entity/order.h
core/session.h
core/repository/i_user_repository.h
core/repository/i_order_repository.h
data/user_dao.h / .cpp            ← 密码哈希写在这里
data/order_dao.h / .cpp
services/auth_service.h / .cpp
services/order_service.h / .cpp
ui/pages/auth/login_dialog.h / .cpp        ← 登录 + 注册（一个对话框两个模式）
ui/pages/order/my_orders_page.h / .cpp     ← 我的订单
ui/pages/order/buy_dialog.h / .cpp         ← 点"购买"后的确认框
ui/pages/order/order_detail_dialog.h / .cpp
```

---

## 任务清单

| # | 任务 | 周 | 前置 | 完成 |
|---|---|---|---|---|
| 1 | 环境 + 分支 `feature/B-order` | 0 | — | ☐ |
| 2 | `user.h` + `order.h` + `session.h` | 0 | 契约 §6 | ☐ |
| 3 | 两个 Repository 接口 | 0 | 2 | ☐ |
| 4 | 提 PR 让 D review | 0 | 3 | ☐ |
| 5 | `UserDao::insert` / `findByUsername` / `findById` / `exists` | 1 | D 建表 | ☐ |
| 6 | 密码 `sha256(盐+密码)` + 随机盐 | 1 | 5 | ☐ |
| 7 | `OrderDao::insert` + `nextOrderNo` | 1 | 5 | ☐ |
| 8 | `OrderDao::updateStatus`（三参数） | 1 | 7 | ☐ |
| 9 | `OrderDao::findById` / `search` | 1 | 7 | ☐ |
| 10 | `AuthService::login` / `registerUser` | 2 | 5,6 | ☐ |
| 11 | 登录/注册对话框 | 2 | 10 | ☐ |
| 12 | **`OrderService::createOrder`（跨域事务）** | 2 | 7,8, A 的 updateStatus | ☐ |
| 13 | `buy_dialog`：确认购买 | 2 | 12 | ☐ |
| 14 | 我的订单页（我买到的 / 我卖出的） | 2 | 9 | ☐ |
| 15 | 订单详情对话框（含该单日志表） | 2 | 9, C 的状态机 | ☐ |
| 16 | 和 A/C 联调 | 3 | 12 | ☐ |

---

## 第 0 周

### 1. 分支

```bash
git checkout develop && git pull
git checkout -b feature/B-order
```

### 2. 三个头文件

`user.h`、`order.h` 照契约 §6 抄。`session.h` 单独一个类：

```cpp
#pragma once
#include "core/entity/user.h"

class Session {
public:
    static Session& instance() { static Session s; return s; }
    const User& currentUser() const { return m_user; }
    bool isLoggedIn() const { return m_user.id > 0; }
    void setCurrentUser(const User& u) { m_user = u; }
    void clear() { m_user = User(); }
private:
    User m_user;
};
```

> 全局单例，谁都能 `Session::instance().currentUser().id`，不用到处传参数。

### 3. 两个接口

```cpp
// i_user_repository.h
class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual OpResult insert(User& u) = 0;          // u.passwordHash 传明文，DAO 内部哈希
    virtual std::optional<User> findByUsername(const QString& name) = 0;
    virtual std::optional<User> findById(int id) = 0;
    virtual bool exists(const QString& name) = 0;
};

// i_order_repository.h
class IOrderRepository {
public:
    virtual ~IOrderRepository() = default;
    virtual OpResult insert(Order& o) = 0;
    virtual OpResult updateStatus(int orderId, const QString& from, const QString& to) = 0;
    virtual std::optional<Order> findById(int id) = 0;
    virtual QVector<Order> search(const OrderQuery& q) = 0;
    virtual QString nextOrderNo() = 0;
};
```

---

## 第 1 周

### 5–6. `UserDao` + 密码

```cpp
static QString sha256Hex(const QString& s)
{
    return QString(QCryptographicHash::hash(s.toUtf8(), QCryptographicHash::Sha256).toHex());
}

// 注意：insert 收到的 User.passwordHash 里是**明文**，出口一律哈希
OpResult UserDao::insert(User& u)
{
    if (u.salt.isEmpty()) u.salt = QUuid::createUuid().toString(QUuid::Id128);
    const QString hash = sha256Hex(u.salt + u.passwordHash);

    QSqlQuery q(DbManager::db());
    q.prepare("INSERT INTO users(username,password_hash,salt,nickname) VALUES(?,?,?,?)");
    q.addBindValue(u.username);
    q.addBindValue(hash);
    q.addBindValue(u.salt);
    q.addBindValue(u.nickname);
    if (!q.exec()) return OpResult::fail("注册失败：" + q.lastError().text());
    u.id = q.lastInsertId().toInt();
    u.passwordHash = hash;          // 顺手把内存里的明文换掉
    return OpResult::success(QString::number(u.id));
}
```

校验密码时**不能只比哈希**，要用库里那条记录的盐重新算：

```cpp
bool UserDao::verifyPassword(const User& u, const QString& plain)
{
    return sha256Hex(u.salt + plain) == u.passwordHash;
}
```

（`verifyPassword` 和 `sha256Hex` 都放 `user_dao.cpp`，声明成 static 或写在头文件里，不加进接口。）

✅ **验证**：注册一个账号，用 DB Browser 打开 `users` 表，`password_hash` 里看不到 `123456`。

### 7. `OrderDao::insert` + `nextOrderNo`

```cpp
QString OrderDao::nextOrderNo()
{
    const QString day = QDate::currentDate().toString("yyyyMMdd");
    QSqlQuery q(DbManager::db());
    q.prepare("SELECT COUNT(*) FROM orders WHERE order_no LIKE ?");
    q.addBindValue(day + "-%");
    q.exec(); q.next();
    const int n = q.value(0).toInt() + 1;
    return day + "-" + QString("%1").arg(n, 4, 10, QChar('0'));   // 20251022-0001
}
```

```cpp
OpResult OrderDao::insert(Order& o)
{
    QSqlQuery q(DbManager::db());
    q.prepare("INSERT INTO orders(order_no,product_id,buyer_id,seller_id,amount,status,remark) "
              "VALUES(?,?,?,?,?,?,?)");
    q.addBindValue(o.orderNo);
    q.addBindValue(o.productId);
    q.addBindValue(o.buyerId);
    q.addBindValue(o.sellerId);
    q.addBindValue(o.amount);
    q.addBindValue(Const::OrderStatus::PendingPay);
    q.addBindValue(o.remark);
    if (!q.exec()) return OpResult::fail("创建订单失败：" + q.lastError().text());
    o.id = q.lastInsertId().toInt();
    return OpResult::success(QString::number(o.id));
}
```

⚠️ `nextOrderNo()` 放在事务**外面**算（`COUNT(*)` 在同一个事务里也行，但先算好更简单）。单机版不会有并发重号。

### 8. `OrderDao::updateStatus` —— 和 A 的写法一模一样

```cpp
q.prepare("UPDATE orders SET status=?, updated_at=datetime('now','localtime') "
          "WHERE id=? AND status=?");
q.addBindValue(to); q.addBindValue(orderId); q.addBindValue(from);
if (q.numRowsAffected() != 1) return OpResult::fail("订单状态已变，请刷新");
```

### 9. `search`（一条 SQL 同时服务"我买到的"和"我卖出的"）

```sql
SELECT o.*, p.title AS product_title, b.nickname AS buyer_name, s.nickname AS seller_name
FROM orders o
LEFT JOIN products p ON p.id = o.product_id
LEFT JOIN users b ON b.id = o.buyer_id
LEFT JOIN users s ON s.id = o.seller_id
WHERE 1=1
```

条件按需拼：`o.buyer_id = ?` / `o.seller_id = ?` / `o.status = ?` / `(o.order_no LIKE ? OR p.title LIKE ?)`，
最后 `ORDER BY o.created_at DESC`。

> **别写两个 DAO 函数**：买家页传 `q.buyerId`，卖家页传 `q.sellerId`。

---

## 第 2 周

### 10. `AuthService`

```cpp
OpResult AuthService::login(const QString& username, const QString& password)
{
    if (username.trimmed().isEmpty()) return OpResult::fail("请输入用户名");
    auto u = m_repo->findByUsername(username.trimmed());
    if (!u || !UserDao::verifyPassword(*u, password))
        return OpResult::fail("用户名或密码错误");     // 不区分是哪个错
    Session::instance().setCurrentUser(*u);
    emit loginSucceeded();
    return OpResult::success();
}

OpResult AuthService::registerUser(const QString& username, const QString& nickname,
                                   const QString& pwd, const QString& pwd2)
{
    if (username.trimmed().size() < 3) return OpResult::fail("用户名至少 3 个字符");
    if (pwd.size() < 6)                return OpResult::fail("密码至少 6 位");
    if (pwd != pwd2)                   return OpResult::fail("两次密码不一致");
    if (m_repo->exists(username.trimmed())) return OpResult::fail("用户名已被占用");

    User u;
    u.username = username.trimmed();
    u.nickname = nickname.trimmed().isEmpty() ? u.username : nickname.trimmed();
    u.passwordHash = pwd;              // 明文交给 DAO，它负责哈希
    auto r = m_repo->insert(u);
    if (!r.ok) return r;
    Session::instance().setCurrentUser(u);
    emit loginSucceeded();
    return OpResult::success();
}
```

### 11. 登录对话框

一个 `QDialog`，两个模式（`m_isRegister` 控制显示"昵称/确认密码"两个控件）。
成功后 `accept()`，主窗口接 `loginSucceeded` 更新标题栏、导航可见性。

⚠️ 坑：主窗口启动时先弹登录框，用户点关闭（`reject`）就直接 `qApp->quit()`，否则会进到没登录的主界面。

### 12. `createOrder` —— **本项目最核心的一段代码**

```cpp
OpResult OrderService::createOrder(int productId, int buyerId,
                                   const QString& remark, int* outOrderId)
{
    auto& db = DbManager::db();

    // 事务外先读，省得事务里拿着空指针
    auto p = m_productRepo->findById(productId);
    if (!p)                                    return OpResult::fail("商品不存在");
    if (p->status != Const::ProductStatus::OnSale)
        return OpResult::fail("该商品已售出或已下架");
    if (p->sellerId == buyerId)                return OpResult::fail("不能购买自己发布的商品");

    if (!db.transaction())                     return OpResult::fail("开启事务失败");

    // 1) 建订单（我的表）
    Order o;
    o.productId = productId;
    o.buyerId   = buyerId;
    o.sellerId  = p->sellerId;
    o.amount    = p->price;
    o.remark    = remark;
    o.orderNo   = m_orderRepo->nextOrderNo();
    auto r = m_orderRepo->insert(o);
    if (!r.ok) { db.rollback(); return r; }

    // 2) 商品置为已售出（A 的表，条件更新：只有还在售才改得动）
    r = m_productRepo->updateStatus(productId, Const::ProductStatus::OnSale,
                                               Const::ProductStatus::Sold);
    if (!r.ok) { db.rollback(); return OpResult::fail("该商品刚刚被买走了，请刷新"); }

    // 3) 写第一条状态日志（C 的表）
    TradeLog log;
    log.orderId    = o.id;
    log.fromStatus = "";                       // 下单前没有状态
    log.toStatus   = Const::OrderStatus::PendingPay;
    log.action     = Const::TradeAction::Create;
    log.operatorId = buyerId;
    log.remark     = "下单";
    r = m_logRepo->insert(log);
    if (!r.ok) { db.rollback(); return r; }

    if (!db.commit()) { db.rollback(); return OpResult::fail("提交事务失败"); }

    if (outOrderId) *outOrderId = o.id;
    emit orderCreated(o.id);
    return OpResult::success(QString::number(o.id));
}
```

**这段答辩必讲**：三张表分属三个人，写在同一个事务里；第 2 步的条件更新保证连点两次只有一次成功。

✅ **验证**：
1. 买一件商品 → 订单出现在"我买到的"→ 商品从检索页消失
2. 再点一次同一件商品 → 提示"该商品已售出或已下架"
3. 买自己发布的商品 → 被拦

### 13. `buy_dialog`

显示：商品标题、价格、卖家昵称、一个备注输入框、`购买 / 取消`。
点购买 → 调 `createOrder` → 成功就 `accept()` 并提示"下单成功，订单号 xxx"。

由 D 在 `mainwindow.cpp` 里接 A 的 `buyRequested(int productId)`。

### 14. 我的订单页

顶部两个控件：`QComboBox`（我买到的 / 我卖出的）、`QComboBox`（全部状态 / 待付款 / …）。
下面 `QTableWidget`：订单号、商品、金额、状态、对方、创建时间。双击一行 → 订单详情。

切换下拉就重查：

```cpp
OrderQuery q;
if (ui->roleCombo->currentIndex() == 0) q.buyerId  = Session::instance().currentUser().id;
else                                    q.sellerId = Session::instance().currentUser().id;
q.status = ui->statusCombo->currentData().toString();   // "全部"用空串
refresh(m_orderService->queryOrders(q));
```

### 15. 订单详情对话框

上半部分显示订单信息（订单号、商品、金额、买家、卖家、当前状态）。
中间**一张小的 `QTableWidget`** 显示该单的日志（时间 / 从 → 到 / 操作人 / 备注，用 `TradeService::orderLogs(id)`）——**不做花哨的时间线控件**。

底部按钮**由 C 的状态机决定**，你不写任何状态判断：

```cpp
for (const QString& action : TradeStateMachine::actionsFor(order.status)) {
    auto* btn = new QPushButton(TradeStateMachine::actionName(action), this);
    connect(btn, &QPushButton::clicked, this, [=] {
        auto r = m_tradeService->changeStatus(order.id, action,
                                              Session::instance().currentUser().id);
        if (!r.ok) QMessageBox::warning(this, "操作失败", r.message);
        else       { refreshAll(); }
    });
    ui->btnLayout->addWidget(btn);
}
```

> 注意：`actionsFor` 只告诉你"这个状态理论上能做哪些动作"，**具体这个人能不能做**由 `changeStatus` 里面的权限校验兜底。所以卖家的详情页上也会看到"付款"按钮，点了会提示"只有买家可以执行此操作"——这可以接受，简单。

✅ **验证**：完成状态的订单，一个操作按钮都没有。

---

## 第 3 周

- 和 A 联调：下单后商品消失、取消后回来
- 和 C 联调：详情页按钮点了状态真的变、日志表多一条
- 修自己的 bug

---

## 验收清单

- [ ] 注册 → 登录 → 主界面标题显示当前用户 → 退出登录回到登录框
- [ ] 库里看不到明文密码
- [ ] 下单成功：订单在"我买到的"里，卖家在"我卖出的"里能看到
- [ ] 连点两次购买，第二次被拦，库里只有一条订单
- [ ] 不能买自己的商品
- [ ] 订单详情能看到日志表；终态订单没有操作按钮

---

## 答辩时你要说的三句话

1. "下单一共写了三张表——订单、商品状态、状态日志，分属三个人。它们在一个事务里，任何一步失败全部回滚，所以不会出现'有订单但商品还在卖'。"
2. "防止重复下单不是靠界面禁用按钮，是靠商品那条 `UPDATE ... WHERE status='ON_SALE'` 的条件更新加影响行数判断，**数据库是最后一道防线**。"
3. "密码存的是 `sha256(盐 + 密码)`，每个用户一个随机盐，库里看不到明文，登录时用同一个盐重算再比。"

---

## 卡住了找谁

| 问题 | 找谁 |
|---|---|
| 事务回滚了但数据还在 | D（`DbManager` / 外键开关） |
| 商品状态改不动 | A（`updateStatus` 的 `from` 传错了） |
| 详情页按钮不全 / 该有的没有 | C（状态机表） |
| 登录框关不掉、主界面进去了 | 自己：`if (dlg.exec() != QDialog::Accepted) return 或 qApp->quit();` |
