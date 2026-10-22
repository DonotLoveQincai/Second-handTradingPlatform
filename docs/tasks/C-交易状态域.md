# 任务表 C · 交易状态域（流转 / 跟踪 / 记录查询）

> **一句话**：订单从"待付款"走到"已完成"，每一步都留痕，还能全平台查。
> **你不用懂**：商品怎么发布（A）、登录怎么做的（B）、窗口怎么搭的（D）。
> **你依赖 D**：`DbManager`、`OpResult`、`Const::`、主窗口挂载点。
> **你依赖 A/B**：`IProductRepository::updateStatus`（取消时把商品放回在售）、`IOrderRepository::updateStatus` / `findById`。
> **别人依赖你**：B 的订单详情页用你的 `TradeStateMachine::actionsFor` 决定显示哪些按钮、用 `TradeService::orderLogs` 显示日志。

---

## 你要改的文件（只有这些）

```
core/entity/trade_log.h
core/repository/i_trade_log_repository.h
data/trade_log_dao.h / .cpp
services/trade_state_machine.h / .cpp     ← 纯逻辑，最容易满分的一部分
services/trade_service.h / .cpp
ui/pages/trade/trade_log_page.h / .cpp    ← 交易记录查询页
```

---

## 任务清单

| # | 任务 | 周 | 前置 | 完成 |
|---|---|---|---|---|
| 1 | 环境 + 分支 `feature/C-trade` | 0 | — | ☐ |
| 2 | `trade_log.h` + `ITradeLogRepository` | 0 | 契约 §6 | ☐ |
| 3 | 提 PR 让 D review | 0 | 2 | ☐ |
| 4 | `TradeLogDao::insert` | 1 | D 建表 | ☐ |
| 5 | `TradeLogDao::findByOrderId`（按 id 升序） | 1 | 4 | ☐ |
| 6 | `TradeLogDao::search` | 1 | 4 | ☐ |
| 7 | **`TradeStateMachine`：那张 5×4 的表** | 1 | 2 | ☐ |
| 8 | `selfCheck()` 穷举自检 | 1 | 7 | ☐ |
| 9 | `TradeService::changeStatus`（事务 + 权限） | 2 | 4,5, A/B 的接口 | ☐ |
| 10 | `TradeService::orderLogs` / `queryLogs` | 2 | 5,6 | ☐ |
| 11 | 交易记录页（关键字 + 状态筛选） | 2 | 6,10 | ☐ |
| 12 | 和 B 联调：详情页按钮 + 日志表 | 3 | 9 | ☐ |
| 13 | 和 A 联调：取消订单商品回到在售 | 3 | 9 | ☐ |

---

## 第 0 周

### 1. 分支

```bash
git checkout develop && git pull
git checkout -b feature/C-trade
```

### 2. 头文件与接口

`trade_log.h` 照契约 §6 抄。接口：

```cpp
class ITradeLogRepository {
public:
    virtual ~ITradeLogRepository() = default;
    virtual OpResult insert(TradeLog& log) = 0;
    virtual QVector<TradeLog> findByOrderId(int orderId) = 0;
    virtual QVector<TradeLog> search(const TradeLogQuery& q) = 0;
};
```

> **DAO 一律不开事务**。事务由调用者（B 的 `createOrder`、你的 `changeStatus`）开，你只负责往里写一条。

---

## 第 1 周

### 4. `TradeLogDao::insert`

```cpp
OpResult TradeLogDao::insert(TradeLog& log)
{
    QSqlQuery q(DbManager::db());
    q.prepare("INSERT INTO trade_log(order_id,from_status,to_status,action,operator_id,remark) "
              "VALUES(?,?,?,?,?,?)");
    q.addBindValue(log.orderId);
    q.addBindValue(log.fromStatus);     // 下单时传空串 ""
    q.addBindValue(log.toStatus);
    q.addBindValue(log.action);
    q.addBindValue(log.operatorId);
    q.addBindValue(log.remark);
    if (!q.exec()) return OpResult::fail("写状态日志失败：" + q.lastError().text());
    log.id = q.lastInsertId().toInt();
    return OpResult::success();
}
```

### 5. `findByOrderId` —— **务必 ORDER BY id，不要 ORDER BY created_at**

```sql
SELECT l.*, u.nickname AS operator_name
FROM trade_log l
LEFT JOIN users u ON u.id = l.operator_id
WHERE l.order_id = ?
ORDER BY l.id ASC
```

⚠️ **坑**：`created_at` 是秒级精度，同一秒内连点两个动作（比如"付款"后马上"发货"）时间戳完全一样，
按时间排序会**乱序**，日志看起来像倒着的。用自增主键 `id` 才是真正的先后顺序。

### 6. `search`（全平台记录查询）

```sql
SELECT l.*, u.nickname AS operator_name, o.order_no
FROM trade_log l
LEFT JOIN users u ON u.id = l.operator_id
LEFT JOIN orders o ON o.id = l.order_id
WHERE 1=1
```

按需拼：`o.order_no LIKE ? OR l.remark LIKE ?`、`l.to_status = ?`，最后 `ORDER BY l.id DESC`（查询页最新的在最上面）。

### 7. `TradeStateMachine` —— 整个项目唯一"有算法"的地方

用一个 `QHash<QString, QHash<QString, QString>>` 表示 `当前状态 → (动作 → 目标状态)`：

```cpp
// trade_state_machine.h
class TradeStateMachine {
public:
    static bool    canTransit(const QString& from, const QString& action);
    static QString nextStatus(const QString& from, const QString& action);   // 非法返回空串
    static QStringList actionsFor(const QString& status);                    // 这个状态能做哪些动作
    static QString actionName(const QString& action);                        // 中文名，按钮用
    static QString statusName(const QString& status);                        // 中文名，表格用
    static void    selfCheck();                                              // 断言用
};
```

```cpp
// .cpp
static const QHash<QString, QHash<QString, QString>>& table()
{
    static const QHash<QString, QHash<QString, QString>> t = {
        { Const::OrderStatus::PendingPay, {
            { Const::TradeAction::Pay,     Const::OrderStatus::Paid      },
            { Const::TradeAction::Cancel,  Const::OrderStatus::Cancelled },
        }},
        { Const::OrderStatus::Paid, {
            { Const::TradeAction::Ship,    Const::OrderStatus::Shipped   },
            { Const::TradeAction::Cancel,  Const::OrderStatus::Cancelled },
        }},
        { Const::OrderStatus::Shipped, {
            { Const::TradeAction::Confirm, Const::OrderStatus::Completed },
        }},
        { Const::OrderStatus::Completed, { }},   // 终态
        { Const::OrderStatus::Cancelled, { }},   // 终态
    };
    return t;
}

bool TradeStateMachine::canTransit(const QString& from, const QString& action)
{
    return table().value(from).contains(action);
}

QString TradeStateMachine::nextStatus(const QString& from, const QString& action)
{
    return table().value(from).value(action);           // 非法时返回空串，正好当"失败"
}

QStringList TradeStateMachine::actionsFor(const QString& status)
{
    return table().value(status).keys();
}
```

`actionName` / `statusName` 用两个 `QHash<QString,QString>` 映射成中文（"付款""发货""确认收货""取消订单"；"待付款""已付款""已发货""已完成""已取消"）。

状态与动作一览（和契约 §8 一字不差）：

| 当前状态 | 能做的动作 | 动作后 |
|---|---|---|
| 待付款 PENDING_PAY | 付款 pay / 取消 cancel | 已付款 / 已取消 |
| 已付款 PAID | 发货 ship / 取消 cancel | 已发货 / 已取消 |
| 已发货 SHIPPED | 确认收货 confirm | 已完成 |
| 已完成 COMPLETED | — | 终态 |
| 已取消 CANCELLED | — | 终态 |

### 8. `selfCheck()`

```cpp
void TradeStateMachine::selfCheck()
{
    int legal = 0, checked = 0;
    const QStringList allStatus = { Const::OrderStatus::PendingPay, Const::OrderStatus::Paid,
                                    Const::OrderStatus::Shipped, Const::OrderStatus::Completed,
                                    Const::OrderStatus::Cancelled };
    const QStringList allAction = { Const::TradeAction::Pay, Const::TradeAction::Ship,
                                    Const::TradeAction::Confirm, Const::TradeAction::Cancel };
    for (const auto& s : allStatus)
        for (const auto& a : allAction) {
            ++checked;
            if (canTransit(s, a)) {
                Q_ASSERT(!nextStatus(s, a).isEmpty());
                ++legal;
            }
        }
    Q_ASSERT(checked == 20);
    Q_ASSERT(legal == 5);      // 5 条合法边，多一条少一条都是表写错了
}
```

在 `main.cpp` 里 `#ifdef QT_DEBUG` 调一次（或者让 D 调）。**答辩时这个是加分项**：
"状态机的合法性我用穷举断言验证过，5 个状态 × 4 个动作共 20 种组合，其中恰好 5 条合法转移。"

---

## 第 2 周

### 9. `TradeService::changeStatus` —— 项目第二段关键代码

```cpp
OpResult TradeService::changeStatus(int orderId, const QString& action, int operatorId)
{
    auto order = m_orderRepo->findById(orderId);
    if (!order) return OpResult::fail("订单不存在");

    // ① 状态机校验（不问数据库，纯逻辑）
    const QString from = order->status;
    if (!TradeStateMachine::canTransit(from, action))
        return OpResult::fail("当前状态不能执行该操作");
    const QString to = TradeStateMachine::nextStatus(from, action);

    // ② 权限校验（按买卖双方身份推导，库里没有"角色"字段）
    auto r = checkPermission(*order, action, operatorId);
    if (!r.ok) return r;

    // ③ 一个事务里改订单 + 写日志（+ 取消时释放商品）
    auto& db = DbManager::db();
    if (!db.transaction()) return OpResult::fail("开启事务失败");

    r = m_orderRepo->updateStatus(orderId, from, to);         // 条件更新，防并发
    if (!r.ok) { db.rollback(); return OpResult::fail("订单状态已变，请刷新后重试"); }

    TradeLog log;
    log.orderId    = orderId;
    log.fromStatus = from;
    log.toStatus   = to;
    log.action     = action;
    log.operatorId = operatorId;
    log.remark     = TradeStateMachine::actionName(action);
    r = m_logRepo->insert(log);
    if (!r.ok) { db.rollback(); return r; }

    // 取消订单：商品从"已售出"放回"在售"
    if (action == Const::TradeAction::Cancel) {
        r = m_productRepo->updateStatus(order->productId,
                                        Const::ProductStatus::Sold,
                                        Const::ProductStatus::OnSale);
        if (!r.ok) { db.rollback(); return OpResult::fail("释放商品失败，取消失败"); }
    }

    if (!db.commit()) { db.rollback(); return OpResult::fail("提交事务失败"); }
    emit orderStatusChanged(orderId);
    return OpResult::success();
}
```

### 权限规则（`checkPermission`，是 private 成员函数，不进接口）

```cpp
// 交易动作    谁可以做
// pay        买家
// confirm    买家
// ship       卖家
// cancel     待付款时：买家或卖家都能取消
//            已付款后：只有卖家能取消（钱"已经付了"，买家不能自己撤）
//            已发货后：不允许取消（状态机里已经没有这条边）
```

```cpp
OpResult TradeService::checkPermission(const Order& o, const QString& action, int uid)
{
    const bool isBuyer  = (uid == o.buyerId);
    const bool isSeller = (uid == o.sellerId);

    if (action == Const::TradeAction::Pay || action == Const::TradeAction::Confirm)
        return isBuyer ? OpResult::success() : OpResult::fail("只有买家可以执行此操作");

    if (action == Const::TradeAction::Ship)
        return isSeller ? OpResult::success() : OpResult::fail("只有卖家可以执行此操作");

    if (action == Const::TradeAction::Cancel) {
        if (o.status == Const::OrderStatus::PendingPay)
            return (isBuyer || isSeller) ? OpResult::success()
                                         : OpResult::fail("无权取消该订单");
        return isSeller ? OpResult::success() : OpResult::fail("已付款的订单只有卖家可以取消");
    }
    return OpResult::fail("未知操作");
}
```

✅ **验证**（拿三个账号 alice/bob/carol 手动试）：
- bob 买 alice 的商品 → bob 能"付款"，alice 不能
- 付款后 → alice 能"发货""取消"，bob 只能"确认收货"（发货后）
- 已完成/已取消的订单，任何动作都失败

### 10. 两个查询函数

```cpp
QVector<TradeLog> TradeService::orderLogs(int orderId)
{
    TradeLogQuery q; q.orderId = orderId;
    return m_logRepo->search(q);          // 详情页用，只要这一单
}

QVector<TradeLog> TradeService::queryLogs(const TradeLogQuery& q)
{
    return m_logRepo->search(q);          // 记录页用，可按关键字/状态筛
}
```

### 11. 交易记录页

```
订单号[______]  状态[全部▾]  [查询]
┌──────────────────────────────────────────────────────────┐
│ 时间 │ 订单号 │ 从 → 到 │ 动作 │ 操作人 │ 备注          │
└──────────────────────────────────────────────────────────┘
```

`QTableWidget` 5~6 列，`setEditTriggers(NoEditTriggers)`。状态下拉用 `currentData()` 存英文常量、"全部"存空串。
**不做日期范围、不做分页**——数据量就百来条，`LIMIT` 都不用。

✅ **验证**：随便走一单到底，记录页按 id 倒序能看到 4~5 条；按订单号搜只剩那一单；选"已取消"只剩取消记录。

---

## 第 3 周

- 和 B 联调：订单详情页的按钮是"能做什么"，点下去能不能做由你的 `checkPermission` 决定，两边要一起试
- 和 A 联调：取消订单后商品真的回到在售、能被别人再买
- 修 bug：**先把这几条练熟**，答辩现场就演示这个

---

## 验收清单

- [ ] 待付款 → 付款 → 发货 → 确认收货，四步全走通，日志四条
- [ ] 待付款时可以取消，取消后商品回到在售、能被别人买走
- [ ] 已发货的订单没有"取消"按钮（状态机里就没有这条边）
- [ ] 已完成 / 已取消的订单，一个按钮都没有
- [ ] 买家用卖家账号点"发货"被拦住
- [ ] `selfCheck()` 的断言在 Debug 下能过（改坏表就崩）
- [ ] 同一单的多条日志顺序正确（按 id，不按时间）

---

## 答辩时你要说的三句话

1. "订单状态不是散落在各个 `if` 里的，是一张 `当前状态 → 动作 → 目标状态` 的表，5 个状态 × 4 个动作共 20 种组合，其中 5 条合法，我用穷举断言把它钉死了。"
2. "改状态是条件更新 `WHERE id=? AND status=<读到的旧状态>`，如果这期间别人改过，影响行数为 0，直接回滚——所以不会出现两个人同时操作后状态错乱。"
3. "取消订单时把商品放回在售，这一步和改订单状态、写日志在**同一个事务**里，所以不会出现'订单取消了但商品还锁着'。"

---

## 卡住了找谁

| 问题 | 找谁 |
|---|---|
| 按钮该有的没有 / 点了提示不能执行 | 先看状态机表，再看 `checkPermission`，最后看 `order.status` 是不是缓存了旧值 |
| 取消后商品没回到在售 | A（`updateStatus` 的 `from` 必须是 `SOLD`） |
| 日志表不显示 / 顺序乱 | 自己：`ORDER BY l.id ASC` |
| 事务里改了订单但日志没写进去 | B 或 D（`DbManager` 连接是否同一个） |
