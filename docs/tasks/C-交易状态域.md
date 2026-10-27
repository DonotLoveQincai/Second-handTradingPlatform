# C 交易状态域 —— 实现规格

这份文档是 C 这部分代码的唯一依据。**接口签名、SQL、状态表、信号名一个字都不要改**，改了就接不上。代码已按此写完并能编译，后续改动必须仍然满足这里的每一条。

---

## 1. 负责什么

- 一张**状态机表**：订单从哪个状态、执行哪个动作、变成哪个状态。全项目只有这一份定义。
- `TradeService::changeStatus`：状态机校验 + 权限校验 + **一个事务里改订单状态、写日志、（取消时）把商品放回在售**。
- 交易记录：全平台流水查询页 `TradeLogPage`，以及某一单的日志序列（订单详情页用）。

**不负责**：订单本身的增删查（B）、商品状态字段的实现（A）、页面组装与导航（D）。

---

## 2. 文件清单

| 路径 | 现状 | 说明 |
|---|---|---|
| `core/entity/trade_log.h` | 已有 | `TradeLog` + `TradeLogQuery`，不改 |
| `core/repository/i_trade_log_repository.h` | 已有 | 3 个纯虚，不改 |
| `data/trade_log_dao.h/.cpp` | 已有 | 不改 |
| `services/trade_state_machine.h/.cpp` | 已有 | 状态表在这里，改规则只改这里 |
| `services/trade_service.h/.cpp` | 已有 | `changeStatus` / `orderLogs` / `queryLogs` |
| `ui/pages/trade/trade_log_page.h/.cpp` | 已有 | 交易记录页 |

---

## 3. 数据结构与接口（逐字照抄）

### 3.1 实体

```cpp
struct TradeLog
{
    int     id = 0;
    int     orderId = 0;
    int     operatorId = 0;
    QString fromStatus;   // 变更前状态；下单那一条是空串 ""
    QString toStatus;     // 变更后状态
    QString action;       // CREATE / PAY / SHIP / CONFIRM / CANCEL
    QString remark;       // 动作中文名，界面直接显示
    QString createdAt;

    QString orderNo;      // JOIN 填充，不存库
    QString productTitle;
    QString operatorName;
};

struct TradeLogQuery
{
    int     orderId = 0;  // > 0 只看这一单
    QString status;       // 按 to_status 过滤，空 = 不限
    QString keyword;      // 订单号 / 备注 / 操作人昵称 模糊匹配
};
```

### 3.2 仓储接口（不改）

```cpp
class ITradeLogRepository
{
public:
    virtual ~ITradeLogRepository() = default;
    virtual OpResult insert(TradeLog& log) = 0;
    virtual QVector<TradeLog> findByOrderId(int orderId) = 0;   // 按 id 正序
    virtual QVector<TradeLog> search(const TradeLogQuery& q) = 0;
};
```

### 3.3 状态机（纯静态，不 new、不碰数据库）

```cpp
class TradeStateMachine
{
public:
    static bool        canTransit(const QString& from, const QString& action);
    static QString     nextStatus(const QString& from, const QString& action);  // 非法返回空串
    static QStringList actionsFor(const QString& status);   // 当前状态能做的动作
    static QString     actionName(const QString& action);   // 动作码 → 中文
    static QString     statusName(const QString& status);   // 状态码 → 中文（= Const::zh）
    static void        selfCheck();                          // 穷举 20 组，合法必须正好 5 条
};
```

### 3.4 交易服务

```cpp
class TradeService : public QObject
{
    Q_OBJECT
public:
    TradeService(IOrderRepository* orderRepo, IProductRepository* productRepo,
                 ITradeLogRepository* logRepo, QObject* parent = nullptr);

    OpResult changeStatus(int orderId, const QString& action, int operatorId);
    QVector<TradeLog> orderLogs(int orderId);              // 订单详情页
    QVector<TradeLog> queryLogs(const TradeLogQuery& q);   // 交易记录页

signals:
    void orderStatusChanged(int orderId, const QString& newStatus);

private:
    OpResult checkPermission(const Order& o, const QString& action, int uid);
};
```

---

## 4. 状态机表（唯一真相）

`from → (action → to)`，就是 `QHash<QString, QHash<QString, QString>>`：

| 当前状态 | 动作 | 目标状态 |
|---|---|---|
| `PENDING_PAY` 待付款 | `PAY` 付款 | `PAID` 已付款 |
| `PENDING_PAY` | `CANCEL` 取消订单 | `CANCELLED` 已取消 |
| `PAID` 已付款 | `SHIP` 发货 | `SHIPPED` 已发货 |
| `PAID` | `CANCEL` 取消订单 | `CANCELLED` |
| `SHIPPED` 已发货 | `CONFIRM` 确认收货 | `COMPLETED` 已完成 |
| `COMPLETED` / `CANCELLED` | — | 终态，无出边 |

- 合法边共 **5** 条，`selfCheck()` 里 `Q_ASSERT(checked == 20)`（5 状态 × 4 动作）、`Q_ASSERT(legal == 5)`。
- `SHIPPED`/`COMPLETED` **不能取消**（只有一条 `CONFIRM` 出边）。
- `CREATE` 不在表里：下单是 B 的 `createOrder` 直接写日志，不走状态机。
- `statusName()` 直接 `return Const::zh(status);`，不要另写一份映射。
- `actionName()` 映射：`CREATE`「创建订单」/`PAY`「付款」/`SHIP`「发货」/`CONFIRM`「确认收货」/`CANCEL`「取消订单」，未知返回原码。

---

## 5. `changeStatus` 的执行顺序（不能调换）

```cpp
auto order = m_orderRepo->findById(orderId);
if (!order) return OpResult::fail("订单不存在");

const QString from = order->status;
if (!TradeStateMachine::canTransit(from, action))
    return OpResult::fail("当前状态不能执行该操作");      // ① 状态机
const QString to = TradeStateMachine::nextStatus(from, action);

OpResult r = checkPermission(*order, action, operatorId);  // ② 权限
if (!r.ok) return r;

QSqlDatabase& db = DbManager::db();                        // ③ 事务
if (!db.transaction()) return OpResult::fail("开启事务失败");

r = m_orderRepo->updateStatus(orderId, from, to);           // 条件更新
if (!r.ok) { db.rollback(); return OpResult::fail("订单状态已变，请刷新后重试"); }

TradeLog log;
log.orderId = orderId; log.fromStatus = from; log.toStatus = to;
log.action = action;   log.operatorId = operatorId;
log.remark = TradeStateMachine::actionName(action);
r = m_logRepo->insert(log);
if (!r.ok) { db.rollback(); return r; }

if (action == Const::TradeAction::Cancel) {                // ④ 只有取消才释放商品
    r = m_productRepo->updateStatus(order->productId,
                                    Const::ProductStatus::Sold,
                                    Const::ProductStatus::OnSale);
    if (!r.ok) { db.rollback(); return OpResult::fail("释放商品失败，取消失败"); }
}

if (!db.commit()) { db.rollback(); return OpResult::fail("提交事务失败"); }

emit orderStatusChanged(orderId, to);
return OpResult::success();
```

要点：

- 事务里改的**三张表**：`orders`（B 的仓储）、`trade_log`（自己的）、`products`（A 的仓储，仅取消时）。这就是本项目「四人协同」最关键的一段。
- 条件是 `(orderId, from, to)`，由 DAO 拼 `UPDATE orders SET status=? ... WHERE id=? AND status=?` 并检查 `numRowsAffected() == 1`；行数为 0 表示别人抢先改了。
- 日志的 `remark` 一律写动作中文名，不要留空。

---

## 6. 权限规则（库里没有角色字段，靠归属推导）

```cpp
const bool isBuyer  = (uid == o.buyerId);
const bool isSeller = (uid == o.sellerId);
```

| 动作 | 谁可以 | 失败文案 |
|---|---|---|
| `PAY` 付款 | 仅买家 | 「只有买家可以执行此操作」 |
| `CONFIRM` 确认收货 | 仅买家 | 「只有买家可以执行此操作」 |
| `SHIP` 发货 | 仅卖家 | 「只有卖家可以执行此操作」 |
| `CANCEL` 且订单 `PENDING_PAY` | 买家或卖家 | 「无权取消该订单」 |
| `CANCEL` 且订单 `PAID` | 仅卖家 | 「已付款的订单只有卖家可以取消」 |
| 其它 | 无 | 「未知操作」 |

---

## 7. DAO 的 SQL（照抄）

### 7.1 `TradeLogDao::insert`

```cpp
q.prepare("INSERT INTO trade_log(order_id, from_status, to_status, action, operator_id, remark) "
          "VALUES(?, ?, ?, ?, ?, ?)");
q.addBindValue(log.orderId);
q.addBindValue(log.fromStatus);   // 下单时是空串 ""
q.addBindValue(log.toStatus);
q.addBindValue(log.action);
q.addBindValue(log.operatorId);
q.addBindValue(log.remark);
```

成功后 `log.id = q.lastInsertId().toInt();`。**DAO 不开事务**，事务由 `createOrder` / `changeStatus` 开。

### 7.2 查询公共部分

```sql
SELECT l.*, u.nickname AS operator_name, o.order_no, p.title AS product_title
FROM trade_log l
LEFT JOIN users    u ON u.id = l.operator_id
LEFT JOIN orders   o ON o.id = l.order_id
LEFT JOIN products p ON p.id = o.product_id
```

### 7.3 `findByOrderId`

`WHERE l.order_id = ? ORDER BY l.id ASC`

**必须按 `id` 排**：`created_at` 只到秒，同一秒写完下单+付款两条，按时间排会乱序。

### 7.4 `search`

`WHERE 1=1` 后面按需拼结构（值全走 `addBindValue`）：

| 条件非空 | 拼的片段 |
|---|---|
| `orderId > 0` | `AND l.order_id = ?` |
| `!status.isEmpty()` | `AND l.to_status = ?` |
| `!keyword.isEmpty()` | `AND (o.order_no LIKE ? OR l.remark LIKE ? OR u.nickname LIKE ?)`，值 `%关键字%` |

结尾 `ORDER BY l.id DESC`（查询页最新在上，与 `findByOrderId` 的正序相反，这是故意的）。

### 7.5 谁调哪一个（别写反）

| Service 方法 | 走哪个 DAO 方法 | 顺序 | 谁用 |
|---|---|---|---|
| `TradeService::orderLogs(orderId)` | `findByOrderId` | `l.id ASC`（旧 → 新） | 订单详情页的时间线，从上往下读 |
| `TradeService::queryLogs(q)` | `search` | `l.id DESC`（新 → 旧） | 交易记录页，最新一条在最上面 |

写反了不会报错，只会让详情页的时间线倒过来。联调用例 9.11（`tests/integration_test.cpp`）就是靠对比这四条日志的 `toStatus` 序列抓到的。

---

## 8. 界面：`TradeLogPage`（已实现）

```cpp
class TradeLogPage : public QWidget
{
    Q_OBJECT
public:
    explicit TradeLogPage(TradeService* service, QWidget* parent = nullptr);
    void refresh();              // MainWindow::onDataChanged 会调
private:
    void doQuery();
};
```

- 顶部筛选条：`QLineEdit`（placeholder「订单号 / 操作人 / 备注」）+ `QComboBox`（「全部」`itemData` 为空串，其余五项 `itemData` 放英文状态码）+「查询」按钮。
- 表格 6 列：时间 / 订单号 / 状态变化 / 动作 / 操作人 / 备注；`NoEditTriggers` + `SelectRows` + 最后一列拉伸。
- 「状态变化」列：`fromStatus` 为空时显示「— → 待付款」，否则「旧中文 → 新中文」（用 `Const::zh`）。
- 「动作」列用 `TradeStateMachine::actionName(l.action)`。
- `refresh()` 就是 `doQuery()`（全量重查，不做增量）。

订单详情页里的日志表由 B 的 `OrderDetailDialog` 渲染，数据来自 `TradeService::orderLogs(orderId)`，**直接按返回顺序显示**，不要再排序。

---

## 9. 与别人的接缝

| 方向 | 内容 |
|---|---|
| 你用别人的 | `IOrderRepository::findById/updateStatus`（B）、`IProductRepository::updateStatus`（A）、`Const::*`、`DbManager::db()`、`OpResult` |
| 别人用你的 | B 的 `createOrder` 调 `ITradeLogRepository::insert`；B 的 `OrderDetailDialog` 调 `TradeStateMachine::actionsFor/actionName` 与 `TradeService::changeStatus/orderLogs`；D 的 `MainWindow` 建 `new TradeService(m_orderRepo, m_productRepo, m_logRepo, this)`、`new TradeLogPage(m_tradeService, this)`，并 `connect(m_tradeService, &TradeService::orderStatusChanged, this, &MainWindow::onDataChanged)` |

**构造参数顺序不能改**：`(orderRepo, productRepo, logRepo, parent)`。

---

## 10. 硬性禁止

1. 状态流转规则只在 `trade_state_machine.cpp` 的 `table()` 里定义；别处不许出现 `if (status == "PENDING_PAY")`。
2. 状态码、动作码一律用 `Const::OrderStatus::*` / `Const::TradeAction::*`，不许写裸字符串。
3. `changeStatus` 里不许省略事务、不许省略 `CANCEL` 时释放商品那一步。
4. DAO 里不许开事务；`TradeService` 里不许碰 `QWidget`。
5. `actionsFor()` 返回的是**动作码**，按钮文字必须过 `actionName()`，码只用来传给 `changeStatus`。
6. 不许把终态（`COMPLETED`/`CANCELLED`）加出边，尤其不许加「取消已完成订单」。
7. 不许删除 `selfCheck()` 的断言——它是状态表的守门人。

---

## 11. 验收清单

- [ ] `Ctrl+Shift+B` 0 错误；Debug 下 F5 启动，输出面板能看到 `[TradeStateMachine] 自检通过： 5 / 20 条合法转移`。
- [ ] 把 `table()` 里随便删一条边，`Q_ASSERT(legal == 5)` 立刻崩——证明自检有效（验完改回来）。
- [ ] 待付款订单：买家点「付款」→ 状态变已付款，`trade_log` 多一行 `from=PENDING_PAY,to=PAID,action=PAY,remark=付款`。
- [ ] 已付款订单：卖家点「发货」→ 已发货；买家点「确认收货」→ 已完成；再点任何按钮都没有按钮可点。
- [ ] 待付款订单：别人（不是买卖双方）调 `changeStatus(..., PAY, 陌生人id)` → 「只有买家可以执行此操作」。
- [ ] 已付款订单：买家点「取消订单」→ 「已付款的订单只有卖家可以取消」。
- [ ] 卖家取消已付款订单 → 订单变已取消，且商品 `status` 从 `SOLD` 回到 `ON_SALE`（能被别人重新买）。
- [ ] 交易记录页按状态筛「已取消」能查到这条；关键字输入订单号能精确命中。
- [ ] 订单详情里日志顺序：下单 → 付款 → 发货 → 确认收货，时间不倒挂。

---

## 12. 已知坑

| 现象 | 原因 | 处理 |
|---|---|---|
| 取消订单后商品再也卖不掉 | 漏了第 ④ 步，或 `updateStatus(productId, Sold, OnSale)` 参数顺序写反 | 顺序必须是「从 Sold 改成 OnSale」 |
| 状态改了但日志没写 | 日志写在了事务外，或 `commit()` 在写日志之前 | 三步都在事务内，最后统一 commit |
| 同一单出现两条完全同时间的日志，顺序颠倒 | 用 `created_at` 排序 | 用 `l.id ASC` |
| 详情页按钮显示成 `PAY`/`SHIP` | 直接把动作码当文字 | 文字用 `actionName(a)` |
| 别人也能点「发货」 | `checkPermission` 漏了判断，或 `operatorId` 传的是 0 | 传当前登录用户 id，不是 0 |
| 报错「订单状态已变，请刷新后重试」但状态确实没变 | 传的 `from` 是界面缓存里的旧状态 | `from` 必须取 `findById` 刚读出来的状态 |
| `QSqlError: no such table: trade_log` | 表名写成了契约早期版本的 `trade_status_log` | 表名是 `trade_log` |
