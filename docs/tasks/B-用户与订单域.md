# 任务表 B · 用户与订单域（登记 / 下单 / 订单记录）

> **一句话**：谁能登录，谁买了什么，记录查得出来。
> **你不用懂**：商品怎么搜（A）、状态机怎么写的（C）、窗口怎么搭的（D）。
> **你依赖 D**：`DbManager`、`OpResult`、`Const::`、主窗口挂载点。
> **你依赖 A**：`IProductRepository::findById` / `updateStatus`。
> **别人依赖你**：C 改状态要调你的 `IOrderRepository::updateStatus`；所有人读 `Session`。

---

## 这本里没有的东西去哪找

| 你要找的 | 看哪里 |
|---|---|
| 装 Qt / VS、各种编译报错、Git 怎么配置 | `docs/02-环境与协作手册.md` |
| 表结构、字段名、接口签名、状态机表（**权威版本，以它为准**） | `docs/01-契约.md` |
| 第 0 周集体要干的事、D 的施工进度 | `docs/tasks/D-平台底座.md` |

三份文档都在同一个仓库里，路径相对仓库根目录。有冲突时**以 `01-契约.md` 为准**，但改契约要先在群里说。

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

## ⚠️ 写第一个 `.cpp` 之前先看这条

D 已经把主窗口挪进 `ui/`，工程现在能编过。你自己**新增 `.cpp` / `.h` 之后，必须**在 VS 里做一次：

> `扩展` → `Qt VS Tools` → `Open Qt Project File (.pro)` → 选 `Second-handTradingPlatform.pro` → 提示转换时点「是」

不做这一步，一定会报：

```
LNK2019 无法解析的外部符号 "public: __cdecl ProductDao::insert(...)"，函数 ... 中引用了该符号
LNK1120 1 个无法解析的外部命令
```

### 为什么会这样

`.pro` 里 `SOURCES` 只写了 `main.cpp`，其余文件全靠四个 `.pri` 用 `$$files()` **通配目录**（`core.pri` 管 `core/`、`data.pri` 管 `data/`、`services.pri` 管 `services/`、`ui.pri` 管 `ui/`）。

通配符只在 **qmake 跑的那一刻**才扫目录。VS 编译用的是 qmake 生成的 `.vcxproj` —— 一份**文件清单快照**。新文件没重新跑 qmake，就永远进不去。

所以这个错跟"代码写错了"没关系，是**你的文件根本没被编译**。

### `LNK2019` 的三条排查顺序（背下来，以后自己能查）

1. **实现真写了吗** —— 打开那个 `.cpp`，不是空的
2. **文件进工程了吗** —— VS「解决方案资源管理器」里能看到它
3. **进了工程但没重新生成** —— 重新走一次 `Open Qt Project File (.pro)`

90% 是第 3 条。

### 你的文件该放哪个目录（放错目录也是 `LNK2019`）

| 你写的东西 | 必须放在 | 通配它的 `.pri` |
|---|---|---|
| 实体 struct、接口 | `core/entity/`、`core/repository/` | `core/core.pri` |
| DAO | `data/` | `data/data.pri` |
| Service | `services/` | `services/services.pri` |
| 页面、对话框 | `ui/pages/<你的模块名>/` | `ui/ui.pri` |

**不要动** `ui/mainwindow.h`、`ui/mainwindow.cpp`（归 D）；**不要**自己在工程根目录建 `.cpp` —— 根目录只有 `main.cpp` 被显式写进 `SOURCES`，放那里必定编不进去。

## 开工前先看：这次施工真实踩过的九个坑

按"从空工程到能跑起来"的顺序排的，你迟早会撞上一半。每条都写清楚**报错原文**，方便直接搜。

| # | 现象 | 真正的原因 | 怎么办 |
|---|---|---|---|
| 1 | `LNK2019 无法解析的外部符号` + `LNK1120` | 新增的 `.cpp` 没进工程：`.vcxproj` 是 qmake 生成的快照，`.pri` 的通配只在生成那一刻扫目录 | 走一次 `扩展 → Qt VS Tools → Open Qt Project File (.pro)`，再 `Ctrl+Shift+B` |
| 2 | `C4828 字符在当前源字符集中无效` / `C2001 常量中有换行符` | 文件存成了 GBK，而 `.pro` 里开了 `/utf-8` | 仓库根已有 `.editorconfig`，新建文件自动 UTF-8；老文件 `文件 → 高级保存选项` → **UTF-8（无签名）** |
| 3 | `C2039 "toVariant": 不是 "QVariant" 的成员` | 抄来的代码本身就有错，`QVariant` 没有 `toVariant()` | 用 `q.value(n).toString()` / `.toInt()`。**文档里的代码也可能写错** —— 报错说某方法不存在，先查这个方法到底有没有，别急着怀疑自己抄漏了 |
| 4 | 编译 0 错误，但 `F5` / 双击 exe 毫无反应，输出面板一行都没有 | `debug\` 缺 `Qt6Sqld.dll` 和 `sqldrivers\`，程序在加载 Qt 库那一步就退了 | `windeployqt --debug --no-translations <debug 里的 exe>` |
| 5 | `QSQLITE driver not loaded` | 同上，SQLite 插件没在 exe 旁边 | 同上 |
| 6 | `qDebug()` 写了，输出面板却什么都没有 | 那行写在了 `return QApplication::exec();` **之后**，`exec()` 阻塞到最后一个窗口关闭，永远走不到；编译器还会警告 `C4702: 不可访问的代码` | 放在 `QApplication a(argc, argv);` 之后、`exec()` 之前；输出看 `输出 → 调试` 面板，不是 `生成` |
| 7 | 自己新建的 `data/xxx_dao.cpp` 在 `git status` 里根本不出现 | `.gitignore` 里写过 `data/`，它是通配，把源码目录也一起忽略了 | 已改成 `/Second-handTradingPlatform/data/app.db` 这种带完整路径的写法；自检 `git status --short -uall` |
| 8 | `git push` 报 `Recv failure: Connection was reset` / `Connection timed out` | 网络到 GitHub 被中断，git 没走代理 | `git config --global http.proxy http://127.0.0.1:7897`（端口看自己代理软件），`https.proxy` 同样一行；代理关了要 `--unset` 撤销 |
| 9 | 提交完发现漏了文件（比如整个 `docs/`） | `git add` 按**当前所在目录**算，在子目录里 `git add .` 只收那棵子树 | 回仓库根再 `git add -A`，或者写明确路径；提交前用 `git status` 看 `Changes to be committed` 里齐不齐 |

#### 提交前 30 秒自检

```bash
git status --short -uall     # 你新建的文件在里面吗？
git diff --cached --stat     # 这次到底收了几个文件，数量对不对
```

#### 三条硬规矩

1. **动代码前**：`git checkout develop` → `git pull` → `git checkout feature/你的分支` → `git merge develop`
2. **新建文件后**：立刻走一次 `Open Qt Project File (.pro)`，别等编译报错才想起来
3. **收工前**：`F5` 跑一遍、点几下界面，确认不是"编译过了但界面是空的"

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

## 提交与提 PR（每次都要按这个来）

### 提交信息怎么写

固定格式：`<类型>:<说明>`

| 类型 | 什么时候用 | 例子 |
|---|---|---|
| `feat` | 新增功能 | `feat:商品上架与下架` |
| `fix` | 修 bug | `fix:取消订单后商品没回到在售` |
| `docs` | 只改文档 | `docs:补充检索页说明` |
| `test` | 只改测试 | 本项目基本用不到 |

规则：用**英文冒号**、冒号后**不空格**、30 字以内、不加句号、直接写中文。

写"做了什么"，不要写"修改了 xxx.cpp"。

### 每天收工三条命令

```bash
git add -A
git commit -m "feat:买家登记与订单记录"
git push -u origin feature/B-order          # 第一次带 -u，以后直接 git push
```

**一天至少提交一次。** 别攒到最后一天 —— 答辩时老师会看提交记录，一天一个巨型提交看着就像临时抄的。

### 提 PR 的完整步骤

1. 打开 GitHub 仓库页面，顶部会出现黄条 `Compare & pull request`，点它
   （没有黄条就点 `Pull requests` → `New pull request`）
2. 确认方向：`base: develop` ← `compare: feature/B-order`
   **base 一定要是 develop，绝不是 main**
3. 标题照提交格式写，例如 `feat:商品域完成`
4. 描述写两三句：做了什么、怎么测的、有什么没做完
5. 右边 `Reviewers` 选 **D**，然后 `Create pull request`
6. 等 D approve。他要你改，就在本地改完再 `git push`，PR 会自动更新
7. 他 approve 后你自己点 `Merge pull request` → `Confirm merge`
   （不要用 `Squash and merge`，否则你们的提交记录会被压成一条，答辩证据就没了）
8. 合完点 `Delete branch` 删掉远端分支（本地分支留着，你还要继续用）

### 合完之后同步一次

```bash
git checkout develop
git pull
git checkout feature/B-order
git merge develop          # 把别人的改动并进来，早发现冲突早解决
```

### 三条禁令

- **不要** `git push origin develop` 或 `git push origin main` —— 分支保护会直接拒绝，D 也没权限给你开
- **不要** `git push -f` —— 会把别人的提交抹掉
- **不要**在别人的分支上提交

### 冲突了怎么办

```bash
git pull --rebase origin develop
```

VS 会在「解决方案资源管理器」里把冲突文件标红。打开它，手动留下要的内容，把 `<<<<<<<`、`=======`、`>>>>>>>` 这几行删干净，然后：

```bash
git add 冲突的文件
git rebase --continue
git push
```

搞不定就在群里喊 D，**别自己硬删别人的代码**。

### 如果冲突的是 `.vcxproj` / `.vcxproj.filters`

这两个是 Qt VS Tools 从 `.pro` **自动生成**的文件，不是手写的，所以不用一行行去合：

1. 随便选一边（`Accept Current Change` 或 `Accept Incoming Change` 都行）
2. 保存，然后在 VS 里再走一次：`扩展` → `Qt VS Tools` → `Open Qt Project File (.pro)` → 转换时点「是」
3. 这一步会**按磁盘上的文件重新生成**整份工程文件，之前选对选错都无所谓
4. `git add` 这两个文件，`git rebase --continue`

一句话记住：**`.pro` 和目录结构才是真相，`.vcxproj` 只是它生成出来的东西。**

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
