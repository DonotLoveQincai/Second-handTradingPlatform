# 任务表 D · 平台底座（骨架 / 数据库 / 组装 / 导入）

> 📖 **动手的时候打开 [D-详细施工步骤.md](D-详细施工步骤.md)**：那份是逐日、逐文件的完整操作手册（含全部可复制代码）。
> 本文件是任务清单和验收标准，两份配合用。

> **一句话**：把仓库、数据库、窗口、拼装这四件事做好，让另外三个人能开工。
> **你不用懂**：具体业务规则（他们三个各管一块）。
> **别人依赖你**：所有文件先有你的骨头才能长肉。**你是第 0 周最忙的人，必须提前一天交付。**
> **你依赖别人**：几乎没有。你先动，别人才动。

---

## 你要改的文件（只有这些）

```
.gitignore
Second-handTradingPlatform.pro
core/core.pri  data/data.pri  services/services.pri  ui/ui.pri
core/common/opresult.h
core/common/constants.h
sql/001_init.sql
ui/resources/resources.qrc
data/db_manager.h / .cpp
ui/mainwindow.h / .cpp            ← 只归你一个人改，别人碰了必冲突
ui/pages/import/import_page.h / .cpp
services/import_service.h / .cpp
main.cpp
```

---

## 任务清单

| # | 任务 | 周 | 前置 | 完成 |
|---|---|---|---|---|
| 1 | 建 GitHub 仓库 + 分支保护 + 邀请三人 | 0 | — | ☐ |
| 2 | `.gitignore`（**必须忽略 `ui_*.h`**） | 0 | 1 | ☐ |
| 3 | 目录骨架 + 四个 `.pri` + 改 `.pro` | 0 | 1 | ☐ |
| 4 | `opresult.h` + `constants.h` | 0 | 3 | ☐ |
| 5 | `sql/001_init.sql`（5 张表 + 索引 + 初始分类） | 0 | 3 | ☐ |
| 6 | `DbManager`（连库、建表、外键、错误处理） | 0 | 5 | ☐ |
| 7 | `MainWindow` 骨架（导航 + 堆叠页 + 占位页） | 0 | 6 | ☐ |
| 8 | 推送 develop，通知三人可以开分支 | 0 | 7 | ☐ |
| 9 | 组装服务：`buildServices()` + 信号连接 | 2 | A/B/C 的类 | ☐ |
| 10 | 登录流程接进主窗口 | 2 | B 的登录框 | ☐ |
| 11 | 演示数据（3 用户 + 12 商品 + 1 条完整订单） | 2 | 9 | ☐ |
| 12 | `ImportService`（CSV 解析 + 整批事务） | 2 | 6 | ☐ |
| 13 | 批量导入页 | 2 | 12 | ☐ |
| 14 | 窗口标题 / 中文 / 数据库路径检查 | 3 | 9 | ☐ |
| 15 | `windeployqt` 打包 + 换台电脑验证 | 3 | 全部 | ☐ |

---

## 第 0 周（五天，一天一件事，别拖）

### 1. 仓库与分支保护

- GitHub 上建**私有**仓库 `Second-handTradingPlatform`，**不要**勾 "Add a README"（避免和本地打架）
- Settings → Branches → 保护 `main`：勾 `Require a pull request before merging`、`Require approvals: 1`，不勾 bypass
- Settings → Collaborators 邀请另外三个人（**每人用自己的 GitHub 账号**）
- 本地：`git remote add origin <地址>` → `git push -u origin main`
- 开 develop：`git checkout -b develop && git push -u origin develop`，然后在分支保护里**也保护 develop**

### 2. `.gitignore`

```gitignore
# 编译产物
build/
debug/
release/
*.obj  *.o  *.pdb  *.ilk  *.exe  *.dll  *.lib  *.exp
# Qt VS Tools / Qt Creator 生成
ui_*.h  moc_*.cpp  qrc_*.cpp  *.vcxproj.user  *.user
.vs/  .qtcreator/  .qmake.stash  *.qtvscr
Makefile  Makefile.*
# 运行时数据（每个人本地自己生成，绝不提交）
data/
logs/
*.db  *.db-journal  *.db-wal  *.db-shm
```

> **`ui_*.h` 这一行是整个项目最重要的三行之一**。它是 `uic` 从 `.ui` 生成的文件，
> 四个人各自电脑上生成的内容会不一样，只要有一个人提交了它，另外三个人天天冲突。
> 同理 `.vcxproj` 里由 Qt VS Tools 生成的路径片段也不该手改。

### 3. 目录骨架 + `.pri`

先 `git commit --allow-empty` 之类的方式把空目录占住（git 不跟踪空目录），
或者每个目录放一个空 `.cpp`/`.h` 占位，然后在 `Second-handTradingPlatform.pro` 里：

```qmake
QT += core gui widgets sql
CONFIG += c++17
TEMPLATE = app
TARGET = Second-handTradingPlatform

INCLUDEPATH += $$PWD

include(core/core.pri)
include(data/data.pri)
include(services/services.pri)
include(ui/ui.pri)
```

每个 `.pri` 长这样（以 `core/core.pri` 为例）：

```qmake
HEADERS += $$files($$PWD/*.h, true) \
           $$files($$PWD/*/*.h, true)
SOURCES += $$files($$PWD/*.cpp, true) \
           $$files($$PWD/*/*.cpp, true)
```

`ui/ui.pri` 多一行 `FORMS += $$files($$PWD/*.ui, true) \
                            $$files($$PWD/*/*.ui, true)`。

> **用通配符的意义**：别人新增文件**不用改 `.pro`**，`qmake` 重跑一次就自动收进来。
> （如果你用 VS 的 Qt VS Tools，加完文件仍需 `扩展 → Qt VS Tools → Open Qt Project File (.pro)` 重新生成工程。）

### 4. `opresult.h` + `constants.h`

```cpp
// opresult.h
#pragma once
#include <QString>

struct OpResult {
    bool ok = false;
    QString message;
    QString value;                          // 成功时可带一个返回值（如新 id）
    static OpResult success(const QString& v = QString()) { return {true, QString(), v}; }
    static OpResult fail(const QString& m)                { return {false, m, QString()}; }
};
```

```cpp
// constants.h —— 不用 enum，用字符串常量，数据库里一眼看得懂
#pragma once
#include <QString>
#include <QHash>

namespace Const {
namespace ProductStatus {
    inline const QString OnSale  = "ON_SALE";
    inline const QString OffSale = "OFF_SALE";
    inline const QString Sold    = "SOLD";
}
namespace OrderStatus {
    inline const QString PendingPay = "PENDING_PAY";
    inline const QString Paid       = "PAID";
    inline const QString Shipped    = "SHIPPED";
    inline const QString Completed  = "COMPLETED";
    inline const QString Cancelled  = "CANCELLED";
}
namespace TradeAction {
    inline const QString Create  = "CREATE";
    inline const QString Pay     = "PAY";
    inline const QString Ship    = "SHIP";
    inline const QString Confirm = "CONFIRM";
    inline const QString Cancel  = "CANCEL";
}
// 状态码 → 中文，界面只调这个
inline QString zh(const QString& code)
{
    static const QHash<QString, QString> m = {
        {"ON_SALE","在售"}, {"OFF_SALE","已下架"}, {"SOLD","已售出"},
        {"PENDING_PAY","待付款"}, {"PAID","已付款"}, {"SHIPPED","已发货"},
        {"COMPLETED","已完成"}, {"CANCELLED","已取消"}
    };
    return m.value(code, code);
}
} // namespace Const
```

> 用 `QString` 常量而不是 `enum class`，是为了答辩时能直接打开数据库指着 `status` 列说
> "这里存的 `ON_SALE` 就是常量 `Const::ProductStatus::OnSale`"——不用解释 0/1/2 是什么意思。

### 5. `sql/001_init.sql`

照契约 §4 原样抄，**一个字都别自己改**（三张任务表都按它写）。要点：

- `PRAGMA foreign_keys = ON;` 写在最前面（但它只对当前连接生效，`DbManager` 里还要再执行一次）
- 每张表后面跟 `CREATE INDEX`（`products(status)`、`orders(buyer_id)`、`orders(seller_id)`、`trade_log(order_id)`）
- 初始分类：5 个父类 + 3 个子类，**id 写死 1~8**（A/B/C 的代码和文档都按这个数字写）

### 6. `DbManager`

```cpp
// db_manager.h
#pragma once
#include <QSqlDatabase>
class DbManager {
public:
    static bool init(QString* errMsg = nullptr);   // 建目录、开库、建表
    static QSqlDatabase& db();
};
```

```cpp
// db_manager.cpp
static QString dbPath()
{
    return QCoreApplication::applicationDirPath() + "/data/app.db";
}

QSqlDatabase& DbManager::db() { static QSqlDatabase d = QSqlDatabase::database("main"); return d; }

bool DbManager::init(QString* errMsg)
{
    QDir().mkpath(QCoreApplication::applicationDirPath() + "/data");

    auto d = QSqlDatabase::addDatabase("QSQLITE", "main");
    d.setDatabaseName(dbPath());
    if (!d.open()) { if (errMsg) *errMsg = "打开数据库失败：" + d.lastError().text(); return false; }

    QSqlQuery q(d);
    q.exec("PRAGMA foreign_keys = ON");       // 每个连接都要设，外键才生效
    q.exec("PRAGMA journal_mode = WAL");

    q.exec("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='users'");
    if (q.next() && q.value(0).toInt() == 0) {
        if (!runSqlScript(d, ":/sql/001_init.sql", errMsg)) return false;
    }
    return true;
}
```

`runSqlScript` = 读资源文件 → 按 `;` 切分 → 逐条 `exec`，**跳过空语句和 `--` 注释行**。
错误时把**出错的那条 SQL 原文**拼进 `errMsg`，否则建表失败你根本不知道错在哪。

⚠️ 坑：`QSQLITE driver not loaded` → 把 `D:\Qt6\6.11.2\msvc2022_64\plugins\sqldrivers\` 整个目录拷到 exe 旁边。

### 7. `MainWindow` 骨架（第 0 周就要能跑起来）

用 `.ui` 拖：左边一个 `QListWidget`（导航，`maximumWidth` 160），右边一个 `QStackedWidget`。
导航项先只加 5 个：`商品检索 / 我的商品 / 我的订单 / 交易记录 / 批量导入`，
每个页面**先放一个 `QWidget` 占位**，中间一个 `QLabel` 写"XXX（待实现）"。

```cpp
// mainwindow.cpp 构造函数里
ui->stackedWidget->addWidget(m_searchPage);   // 第一周这里还是占位 QWidget
connect(ui->navList, &QListWidget::currentRowChanged,
        ui->stackedWidget, &QStackedWidget::setCurrentIndex);
```

然后在群里发一句："**develop 已推，骨架能跑，你们可以开分支了**"。这一天是第 0 周的终点。

---

## 第 2 周（组装 + 导入）

### 9. 组装服务：`buildServices()`

整个项目**只有这一个函数 `new` DAO 和 Service**（术语叫组合根），其他人都不许 `new`：

```cpp
void MainWindow::buildServices()
{
    m_userRepo    = new UserDao(this);      // 注意：DAO/Service 都继承 QObject 传 parent
    m_productRepo = new ProductDao(this);
    m_orderRepo   = new OrderDao(this);
    m_logRepo     = new TradeLogDao(this);

    m_authService    = new AuthService(m_userRepo, this);
    m_productService = new ProductService(m_productRepo, this);
    m_orderService   = new OrderService(m_orderRepo, m_productRepo, m_logRepo, this);
    m_tradeService   = new TradeService(m_orderRepo, m_productRepo, m_logRepo, this);
}

void MainWindow::connectBus()
{
    // 页面之间不直接说话，全部在这里连
    connect(m_searchPage, &SearchPage::buyRequested,
            this, &MainWindow::onBuyRequested);                 // → 弹 B 的 buy_dialog
    connect(m_authService, &AuthService::loginSucceeded,
            this, &MainWindow::onLoginChanged);
    connect(m_authService, &AuthService::logoutDone,
            this, &MainWindow::onLoginChanged);
    connect(m_productService, &ProductService::productsChanged,
            this, &MainWindow::onDataChanged);
    connect(m_tradeService, &TradeService::orderStatusChanged,
            this, &MainWindow::onDataChanged);
}

// 一条数据变了，就让三个列表页各自重查一次。不做增量刷新，简单可靠。
void MainWindow::onDataChanged()
{
    m_searchPage->refresh();
    m_myProductsPage->refresh();
    m_myOrdersPage->refresh();
}
```

> **这是答辩第二必讲的协同点**：四个人的类在这里被拼起来，页面之间只通过信号通信。
> A 的检索页不知道订单存在，B 的订单页不知道商品怎么搜。

### 10. 登录流程

```cpp
// main.cpp 或 MainWindow 构造之后
MainWindow w;
w.show();                                   // 先显示，登录框才有个"父窗口"
LoginDialog dlg;                            // B 的
if (dlg.exec() != QDialog::Accepted) return 0;   // 用户关掉登录框 = 退出程序
w.afterLogin();
return qApp->exec();
```

导航项在未登录时全部 `setEnabled(false)`，登录后启用；标题栏显示 `当前用户：alice`。

### 11. 演示数据（"重置演示数据"菜单，`工具 → 重置演示数据`）

必须**走 DAO**，不能直接 SQL 写死密码哈希：

```cpp
void MainWindow::resetDemoData()
{
    // 1) 三个用户（明文 123456 交给 UserDao::insert，它内部哈希）
    const QVector<QPair<QString,QString>> users = {
        {"alice","Alice"}, {"bob","Bob"}, {"carol","Carol"}
    };
    QHash<QString,int> uid;
    for (auto& u : users) {
        User user; user.username = u.first; user.nickname = u.second;
        user.passwordHash = "123456";
        if (!m_userRepo->exists(u.first)) m_userRepo->insert(user);   // 已存在就跳过，可反复点
        uid[u.first] = user.id;
    }
    // 2) 12 件商品：价格 15~3200，分类分散在 6/7/8/1/2，卖家轮流 alice/bob
    // 3) 一件"进行中"的订单：bob 买了 alice 的一件，状态 PAID，带 2 条日志
    //    —— 这样一开机就有东西可以演示，不用现场从头点
}
```

⚠️ 顺序不能错：**先插用户拿到真实 id，再插商品**（`seller_id` 外键指向 users）。
演示订单也一样：商品 id、买家 id、卖家 id 都得是真实存在的。

### 12. `ImportService`

```cpp
struct ImportReport {
    int total = 0;
    int succeeded = 0;
    QStringList errors;            // "第 3 行：价格不是数字"
};

class ImportService : public QObject {
public:
    ImportReport importCsv(const QString& filePath, int defaultSellerId);
};
```

CSV 表头（固定，第一行）：`title,category,price,condition_level,description`

实现要点：
1. **读文件 + 编码**：先按 UTF-8 解，出现替换字符 `\uFFFD` 就改用 `QStringConverter::System`（Windows 上是 GBK）。这样 Excel 另存的 CSV 也能读。
2. **按行切 + 按逗号切**，处理引号：`"he said ""hi"""` → `he said "hi"`（先扫字符，遇到引号内的逗号不切）。写一个 `parseCsvLine` 就够了，20 行。
3. 跳过表头，逐行校验：标题非空且 ≤60 字、价格 > 0、成色 1~10、分类在 `categories` 里存在（按名字查 id）。
4. **有任意一行出错就整批不导**：`errors` 收集全部错误行（**不是遇到第一个就返回**，让人一次改完），直接返回 `{total, 0, errors}`。
5. 全部通过 → `db.transaction()` → 逐行 `productDao->insert` → 全部成功 `commit()`，任一失败 `rollback()` 并返回错误。

```cpp
// 简化后的核心流程
ImportReport r;
for (每行) {
    ++r.total;
    if (!校验通过) { r.errors << QString("第 %1 行：%2").arg(lineNo).arg(why); continue; }
    rows << p;                     // 先都放进 vector，不落库
}
if (!r.errors.isEmpty()) return r; // 一行有问题，整批不导

db.transaction();
for (auto& p : rows) {
    if (!m_productDao->insert(p).ok) { db.rollback(); r.errors << "写入失败"; return r; }
}
db.commit();
r.succeeded = rows.size();
return r;
```

### 13. 批量导入页

```
[选择 CSV 文件…]  D:/demo.csv
[开始导入]
结果：共 10 行，成功 10 行
（或）
结果：共 10 行，成功 0 行
第 3 行：价格不是数字
第 7 行：分类"游戏机"不存在
```

一个 `QPushButton` + 一个 `QLabel`（文件路径）+ 一个 `QPlainTextEdit`（只读，显示结果）。
**不做预览表格**——直接导、错了就列出来，足够。

---

## 第 3 周

### 14. 收尾

- 窗口标题 `二手交易平台`，`setWindowIcon`
- 确认数据库路径是 `exe目录/data/app.db`（不是当前工作目录——VS 里调试时两者不同，这是经典坑）
- 把所有页面的刷新时机检查一遍：登录后、下单后、改状态后

### 15. 打包

```bash
# 在 release 目录，用 Qt 自带的部署工具
D:\Qt6\6.11.2\msvc2022_64\bin\windeployqt.exe --release --no-translations Second-handTradingPlatform.exe
```

然后**必须确认**这三样在 exe 旁边：
- `Qt6Core.dll` / `Qt6Gui.dll` / `Qt6Widgets.dll` / `Qt6Sql.dll`
- `sqldrivers\qsqlite.dll`  ← **少了它程序起来就报"数据库打不开"，最常见的事故**
- `platforms\qwindows.dll`

最后把整个 `release` 目录拷到**另一台没装 Qt 的电脑**上双击，能开、能搜、能下单，才算完。

---

## 验收清单

- [ ] 四个人 `git clone` 后，什么都不用改，F5 就能跑出空窗口
- [ ] 删掉本地 `data/app.db` 重跑，5 张表自动建出来，分类有 8 条
- [ ] 菜单"重置演示数据"点两次不会因为主键冲突报错
- [ ] 三个页面之间的联动全靠 `mainwindow.cpp` 里的 `connect`
- [ ] 导入 10 行 CSV 成功 10 行；故意改坏一行，成功 0 行并列出错在哪行
- [ ] 拷到没装 Qt 的电脑上能运行

---

## 答辩时你要说的三句话

1. "整个项目只有一个地方 `new` 对象（`buildServices`），一个地方连信号（`connectBus`），页面之间互相不认识，只通过信号和接口通信——四个人的代码才能拼得起来。"
2. "数据库路径用的是 `applicationDirPath()`，不是相对路径，所以从 VS 里调试和双击 exe 运行，用的都是同一个库文件。"
3. "CSV 导入是整批事务：只要有一行不合法，一行都不入，并且把每一行的错误都列出来——不会出现导了一半的脏数据。"

---

## 卡住了找谁

| 问题 | 找谁 |
|---|---|
| `QSQLITE driver not loaded` | 自己：拷 `plugins\sqldrivers` |
| 别人加了文件编译报"找不到头文件" | 自己：`.pri` 通配符没生效，重跑 qmake / 重新生成 VS 工程 |
| 两个页面的信号没连上 | 自己：`connectBus()` 里漏了 |
| 建表 SQL 报错 | 自己：`runSqlScript` 把出错的 SQL 原文打出来 |
| 外键约束不生效 | 自己：`PRAGMA foreign_keys = ON` 是**每连接**的，`init` 里有，换连接就要重设 |
