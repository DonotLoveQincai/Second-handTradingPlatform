# D · 详细施工步骤（从零到能跑）

> 这份是 D 一个人的操作手册：**每一步都写清楚点哪里、抄哪段代码、怎么验证**。
> 第 0 周四天内要全部做完，第五天推 develop 通知其他人。
> 遇到报错先查 [02-环境与协作手册](../02-环境与协作手册.md) 第 7 节。
>
> **约定**：以下所有路径都相对 `Second-handTradingPlatform/`（`.pro` 所在目录）。
> 命令都在 `Git Bash` 里执行，路径用 `/d/...` 风格。

---

# 第一部分 · 第 0 周（四天出骨架，第五天通知全员）

## Day 1 · 建仓库、分支保护、邀请人

### 1.1 在 GitHub 上建仓库

1. 打开 https://github.com/new
2. `Repository name` 填 `Second-handTradingPlatform`
3. 选 **Private**
4. **不要**勾 `Add a README file`、**不要**勾 `Add .gitignore`、**不要**选 license（本地已经有了，勾了会冲突）
5. 点 `Create repository`，把页面上那个 `https://github.com/你的账号/Second-handTradingPlatform.git` 记下来

### 1.2 邀请另外三个人

`仓库页 → Settings → Collaborators → Add people`，输入他们的 GitHub 用户名，三个人都加。
**必须每人一个账号**，答辩要按账号看 Contributors 图。

### 1.3 把本地目录变成 Git 仓库（先写 `.gitignore`，再提交）

`Second-handTradingPlatformDev` 现在只是一个**普通文件夹**，里面还没有 `.git`。先确认：

```bash
cd /d/DSH-workspace/Second-handTradingPlatformDev
ls -a                      # 看不到 .git 就对了
```

**先建 `.gitignore`**（放在 `Second-handTradingPlatformDev/` 下，和 `README.md`、`docs/` 同级）：

```gitignore
# ---- 编译产物 ----
build/
debug/
release/
*.obj
*.o
*.pdb
*.ilk
*.exe
*.dll
*.lib
*.exp
*.idb

# ---- 生成文件（这几行决定四个人会不会天天冲突）----
ui_*.h
moc_*.cpp
qrc_*.cpp
*.vcxproj.user
*.user
*.qtvscr
.qmake.stash

# ---- IDE ----
.vs/
.qtcreator/

# ---- 运行时数据 ----
data/
logs/
*.db
*.db-journal
*.db-wal
*.db-shm

# ---- 系统 ----
Thumbs.db
desktop.ini
```

然后初始化并提交：

```bash
git init
git status                  # 应该列出 docs/ README.md Second-handTradingPlatform/
git add .
git status                  # 🔴 关键：这里不能出现 build/ debug/ *.exe ui_mainwindow.h
git commit -m "chore: 初始化仓库（文档 + 空 Qt 工程）"
```

> **`ui_*.h` 那一行是全文最重要的三行之一**：它由 `uic` 从 `.ui` 生成，四个人的机器生成的内容不一样，
> 谁提交了它，谁就让另外三个人天天冲突。

> ⚠️ 如果 `git status` 里出现了 `build/`、`debug/`、`*.exe`、`ui_mainwindow.h`，说明 `.gitignore` 没生效
> （多半是文件名存成了 `.gitignore.txt`）。改好后执行 `git rm -r --cached build debug` 再 `git add .`。

### 1.4 关联 GitHub 仓库并推送

```bash
git remote add origin https://github.com/你的账号/Second-handTradingPlatform.git
git remote -v                       # 应该显示一行 fetch 一行 push
git branch -M main                  # 把默认分支改名成 main
git push -u origin main
```

第一次推送会弹出浏览器窗口要你登录 GitHub，点 `Authorize` 就行；没有弹窗就按终端提示输入账号和 Personal Access Token。

✅ **验证**：刷新网页，仓库里能看到 `README.md`、`docs/`、`Second-handTradingPlatform/`，**看不到** `build/`。

### 1.5 建 develop 分支并保护它

```bash
git checkout -b develop
git push -u origin develop
git checkout main
```

然后 GitHub 网页上：
`Settings → Branches → Add branch protection rule`

对 `main` 和 `develop` **各做一次**，勾选：

- [x] `Require a pull request before merging`
  - [x] `Require approvals` → 1
- [x] `Do not allow bypassing the above settings`

（其它都不用勾，勾多了自己也会被卡住。）

**然后立刻切到你自己的工作分支**（Day 2~4 的所有提交都在这个分支上，因为 `main` / `develop` 已经被保护，直接 push 会被拒）：

```bash
git checkout -b feature/D-base
git push -u origin feature/D-base
```

> 到这里 Day 1 就结束了。`main` 上只有第一个提交（骨架 + 文档），`develop` 是空的起点，
> 你之后 Day 2~4 的所有提交都进 `feature/D-base`，第五天再一次性 PR 合进 `develop`。

---

## Day 2 · 目录骨架 + `.pro` + 四个 `.pri`

### 2.1 建目录（一次性全建好）

```bash
cd /d/DSH-workspace/Second-handTradingPlatformDev/Second-handTradingPlatform

mkdir -p core/common core/entity core/repository
mkdir -p data
mkdir -p services
mkdir -p ui/pages/product ui/pages/auth ui/pages/order ui/pages/trade ui/pages/import
mkdir -p ui/resources
mkdir -p sql
mkdir -p assets/demo
```

> Git 不跟踪空目录。现在先不提交，等 Day 2.4 放完占位文件再一起提交。

### 2.2 写 `Second-handTradingPlatform.pro`（整个文件替换）

```qmake
QT       += core gui widgets sql
CONFIG   += c++17
TEMPLATE  = app
TARGET    = Second-handTradingPlatform

# 让 #include "core/entity/product.h" 这种写法能找到文件
INCLUDEPATH += $$PWD

# MSVC 下强制按 UTF-8 编译，否则中文字符串会乱码
msvc {
    QMAKE_CXXFLAGS += /utf-8
}

SOURCES += main.cpp

include(core/core.pri)
include(data/data.pri)
include(services/services.pri)
include(ui/ui.pri)

RESOURCES += ui/resources/resources.qrc

# 输出目录固定，避免 VS 和 Creator 各生成一份
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/debug
} else {
    DESTDIR = $$PWD/release
}
```

> `RESOURCES` 只在这里写一次，`ui.pri` 里**不要**再 glob `*.qrc`，否则资源会被注册两遍。

### 2.3 写四个 `.pri`

**`core/core.pri`**

```qmake
HEADERS += $$files($$PWD/*.h, true)
SOURCES += $$files($$PWD/*.cpp, true)
```

**`data/data.pri`**

```qmake
HEADERS += $$files($$PWD/*.h, true)
SOURCES += $$files($$PWD/*.cpp, true)
```

**`services/services.pri`**

```qmake
HEADERS += $$files($$PWD/*.h, true)
SOURCES += $$files($$PWD/*.cpp, true)
```

**`ui/ui.pri`**

```qmake
HEADERS += $$files($$PWD/*.h, true)
SOURCES += $$files($$PWD/*.cpp, true)
FORMS   += $$files($$PWD/*.ui, true)
```

> `$$files(路径, true)` 的第二个参数 `true` = **递归**，所以 `ui/*.h` 会把
> `ui/pages/product/search_page.h` 也收进来，不用为每个子目录写一行。
> 新增文件后**不用改 `.pri`**，只要重新跑一次 qmake / 重新生成 VS 工程。

### 2.4 放占位文件（让目录非空、让空工程能编译）

给每个人建一个只有 `#pragma once` 的空头文件占位，避免目录被 git 忽略：

```bash
# A
echo "#pragma once" > core/entity/product.h
echo "#pragma once" > core/repository/i_product_repository.h
echo "#pragma once" > data/product_dao.h
echo "#pragma once" > services/product_service.h
# B
echo "#pragma once" > core/entity/user.h
echo "#pragma once" > core/entity/order.h
echo "#pragma once" > core/session.h
echo "#pragma once" > core/repository/i_user_repository.h
echo "#pragma once" > core/repository/i_order_repository.h
echo "#pragma once" > data/user_dao.h
echo "#pragma once" > data/order_dao.h
echo "#pragma once" > services/auth_service.h
echo "#pragma once" > services/order_service.h
# C
echo "#pragma once" > core/entity/trade_log.h
echo "#pragma once" > core/repository/i_trade_log_repository.h
echo "#pragma once" > data/trade_log_dao.h
echo "#pragma once" > services/trade_state_machine.h
echo "#pragma once" > services/trade_service.h
```

> ⚠️ 这些占位文件**不要**加 `#include` 也别加类定义。它们的作用只是占住目录 + 让 Day 3 能编译通过。
> 各自的负责人会在自己分支上把它们写成真正的实现，**你不要去改它们的内容**。

### 2.5 验证「空工程能编译」

1. VS 里 `文件 → 打开 → 项目/解决方案` → 选 `Second-handTradingPlatform/Second-handTradingPlatform.pro`
2. 提示转换时点 **是**，等它生成 `.sln` 和 `.vcxproj`
3. 配置选 `Debug` + `x64`
4. `Ctrl+Shift+B` → 应该 0 错误（此时 `main.cpp` 还是模板的老代码，能编过就算成功）

> 这一步失败，99% 是 `.pro` 里 `include()` 的路径写错，或者 `.pri` 文件名拼错。
> qmake 的报错会直接指出是哪一行。

### 2.6 提交 Day 2

```bash
git add .
git commit -m "feat(build): 建立目录骨架、.pro 与四个 .pri"
git push
```

---

## Day 3 · 公共头文件 + 建库 SQL + 资源文件

### 3.1 `core/common/opresult.h`（完整文件）

```cpp
#pragma once

#include <QString>

// 所有写操作的统一返回值。项目不用异常，也不用 std::expected。
struct OpResult
{
    bool    ok = false;
    QString message;   // 失败原因，直接显示给用户
    QString value;     // 成功时可带一个值（新 id、订单号），不需要就留空

    static OpResult success(const QString& v = QString())
    {
        OpResult r;
        r.ok = true;
        r.value = v;
        return r;
    }

    static OpResult fail(const QString& m)
    {
        OpResult r;
        r.ok = false;
        r.message = m;
        return r;
    }
};
```

### 3.2 `core/common/constants.h`（完整文件）

```cpp
#pragma once

#include <QHash>
#include <QString>

// 全项目只在这里出现状态/动作的字面量。别处一律用 Const:: 下的常量。
// 用 QString 而不是 enum：打开数据库能直接看懂 status 列，答辩好讲。
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

// 状态码 → 中文。界面只调这个，不要自己写 if/else 翻译。
inline QString zh(const QString& code)
{
    static const QHash<QString, QString> m = {
        { "ON_SALE",     "在售"   },
        { "OFF_SALE",    "已下架" },
        { "SOLD",        "已售出" },
        { "PENDING_PAY", "待付款" },
        { "PAID",        "已付款" },
        { "SHIPPED",     "已发货" },
        { "COMPLETED",   "已完成" },
        { "CANCELLED",   "已取消" },
    };
    return m.value(code, code);
}

} // namespace Const
```

> 三个子命名空间里的常量都是 `inline const QString`，需要 C++17——`.pro` 里 `CONFIG += c++17` 已经加了。

### 3.3 `sql/001_init.sql`（完整文件，一个字别改）

```sql
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
```

**⚠️ 三条硬规矩**（`runSqlScript` 的实现依赖它们）：

1. **不要写行内注释**（`-- 注释` 只能单独占一行），因为脚本是按 `;` 切的
2. 每条语句以 `;` 结尾，最后一条也要
3. 分类 id 写死 1~8，别人的代码和文档都按这 8 个数字写

### 3.4 `ui/resources/resources.qrc`（完整文件）

```xml
<RCC>
    <qresource prefix="/">
        <file alias="sql/001_init.sql">../../sql/001_init.sql</file>
    </qresource>
</RCC>
```

> 路径是**相对 `.qrc` 文件所在目录**（`ui/resources/`），所以要退两级到工程根。
> 编译后用 `:/sql/001_init.sql` 访问。改了这个文件后必须重新生成工程，否则资源内容不会更新。

### 3.5 验证资源真的进程序了

在 `main.cpp` 里临时加两行（Day 4 会正式用上）：

```cpp
#include <QFile>
#include <QDebug>
...
qDebug() << "sql exists:" << QFile::exists(":/sql/001_init.sql");
```

F5 运行，**输出窗口**要打印 `sql exists: true`。
如果是 `false`，检查：`.qrc` 有没有在 `.pro` 的 `RESOURCES` 里、路径 `../../sql/` 对不对、有没有重新生成工程。

### 3.6 提交 Day 3

```bash
git add .
git commit -m "feat(core): 添加 OpResult、常量定义、建库 SQL 与资源文件"
git push
```

---

## Day 4 · `DbManager`（真正把库建出来）

### 4.1 `data/db_manager.h`（完整文件）

```cpp
#pragma once

#include <QSqlDatabase>
#include <QString>

// 全项目唯一的数据库入口。所有 DAO 都用 DbManager::db() 拿连接。
class DbManager
{
public:
    // 创建 data 目录、打开 app.db、必要时建表。
    // 失败时把原因写进 errMsg（可为 nullptr），返回 false。
    static bool init(QString* errMsg = nullptr);

    // 唯一连接。第一次调用时初始化，之后一直复用。
    static QSqlDatabase& db();

    // 数据库文件的绝对路径（exe 同目录/data/app.db），日志和提示里用
    static QString dbPath();

    // 调试用：打印所有表名和分类条数，仅在 QT_DEBUG 下调用
    static void dumpSchema();
};
```

### 4.2 `data/db_manager.cpp`（完整文件）

```cpp
#include "data/db_manager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

namespace {

// 逐条执行 SQL 脚本。规则：按 ';' 切分，跳过空语句和整行的 "--" 注释。
bool runSqlScript(QSqlDatabase& db, const QString& resourcePath, QString* errMsg)
{
    QFile f(resourcePath);
    if (!f.open(QIODevice::ReadOnly)) {
        if (errMsg) *errMsg = QStringLiteral("无法打开内置 SQL 脚本：%1").arg(resourcePath);
        return false;
    }

    QString text = QString::fromUtf8(f.readAll());
    f.close();

    // 先去掉整行注释，避免注释里的分号把语句切断
    QStringList cleanLines;
    const QStringList rawLines = text.split('\n');
    for (const QString& raw : rawLines) {
        const QString line = raw.trimmed();
        if (line.startsWith("--"))
            continue;
        cleanLines << raw;
    }
    text = cleanLines.join('\n');

    QSqlQuery q(db);
    const QStringList stmts = text.split(';', Qt::SkipEmptyParts);
    for (const QString& raw : stmts) {
        const QString stmt = raw.trimmed();
        if (stmt.isEmpty())
            continue;
        if (!q.exec(stmt)) {
            if (errMsg) {
                *errMsg = QStringLiteral("建表失败：%1\n出错语句：%2")
                              .arg(q.lastError().text(), stmt.left(200));
            }
            return false;
        }
    }
    return true;
}

} // namespace

QString DbManager::dbPath()
{
    // 用 exe 所在目录，不要用当前工作目录：VS 里调试时两者不同，这是经典坑
    return QCoreApplication::applicationDirPath() + QStringLiteral("/data/app.db");
}

QSqlDatabase& DbManager::db()
{
    static QSqlDatabase conn = QSqlDatabase::database(QStringLiteral("main"));
    return conn;
}

bool DbManager::init(QString* errMsg)
{
    const QString dir = QCoreApplication::applicationDirPath() + QStringLiteral("/data");
    if (!QDir().mkpath(dir)) {
        if (errMsg) *errMsg = QStringLiteral("无法创建数据目录：%1").arg(dir);
        return false;
    }

    QSqlDatabase conn = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                  QStringLiteral("main"));
    conn.setDatabaseName(dbPath());

    if (!conn.open()) {
        if (errMsg) {
            *errMsg = QStringLiteral("打开数据库失败：%1\n数据库路径：%2\n"
                                     "若提示 driver not loaded，请把 Qt 的 "
                                     "plugins/sqldrivers 目录拷到 exe 同目录。")
                          .arg(conn.lastError().text(), dbPath());
        }
        return false;
    }

    // 这两个 PRAGMA 是"按连接"生效的，每次打开都要设
    QSqlQuery q(conn);
    q.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    q.exec(QStringLiteral("PRAGMA journal_mode = WAL"));

    // 判断是否已建表：有 users 表就认为初始化过了
    q.exec(QStringLiteral("SELECT COUNT(*) FROM sqlite_master "
                          "WHERE type='table' AND name='users'"));
    const bool hasTables = q.next() && q.value(0).toInt() > 0;

    if (!hasTables) {
        if (!runSqlScript(conn, QStringLiteral(":/sql/001_init.sql"), errMsg))
            return false;
        qDebug() << "[DbManager] 数据库初始化完成：" << dbPath();
    } else {
        qDebug() << "[DbManager] 使用已有数据库：" << dbPath();
    }
    return true;
}

void DbManager::dumpSchema()
{
#ifdef QT_DEBUG
    QSqlDatabase& conn = db();
    qDebug() << "[DbManager] 表：" << conn.tables();
    QSqlQuery q(conn);
    if (q.exec(QStringLiteral("SELECT COUNT(*) FROM categories")) && q.next())
        qDebug() << "[DbManager] 分类条数：" << q.value(0).toInt();
    if (q.exec(QStringLiteral("SELECT id,name,parent_id FROM categories ORDER BY id"))) {
        while (q.next()) {
            qDebug() << "  category" << q.value(0).toInt()
                     << q.value(1).toString()
                     << "parent=" << q.value(2).toVariant().toString();
        }
    }
#endif
}
```

### 4.3 `main.cpp`（完整文件）

```cpp
#include <QApplication>
#include <QMessageBox>

#include "data/db_manager.h"
#include "ui/mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("二手交易平台"));

    // 1) 先把数据库准备好，失败就直接退出，不让用户看到半个窗口
    QString err;
    if (!DbManager::init(&err)) {
        QMessageBox::critical(nullptr, QStringLiteral("初始化失败"), err);
        return 1;
    }
    DbManager::dumpSchema();   // 调试用，正式打包前可注释掉

    // 2) 主窗口（第 0 周只有占位页，第 2 周才会接上真正的页面）
    MainWindow w;
    w.show();

    return app.exec();
}
```

> 登录流程现在先不写（登录框属于 B，第 2 周才有）。第 0 周的目标是"能开窗 + 能建库"。

### 4.4 第一次跑起来（关键验证）

1. `Ctrl+Shift+B` 编译
2. `F5` 运行
3. 应该弹出一个窗口（此时还是模板的样子，正常）
4. **输出窗口**里应该有：

```
[DbManager] 数据库初始化完成： .../debug/data/app.db
[DbManager] 表： ("categories", "orders", "products", "sqlite_sequence", "trade_log", "users")
[DbManager] 分类条数： 8
  category 1 "数码电子" parent=
  category 6 "手机" parent=1
  ...
```

5. 去目录里看：`Second-handTradingPlatform/debug/data/app.db` **真的存在**
6. 关掉程序再运行一次，输出应该变成 `使用已有数据库`（说明不会重复建表）

### 4.5 如果报错，对照这张表

| 输出/现象 | 原因 | 解决 |
|---|---|---|
| `driver not loaded` | SQLite 插件没在 exe 旁边 | 把 `D:\Qt6\6.11.2\msvc2022_64\plugins\sqldrivers\` 整个目录拷到 `debug\` 下 |
| `无法打开内置 SQL 脚本` | 资源没编进去 | `.pro` 里有 `RESOURCES += ui/resources/resources.qrc` 吗；`.qrc` 路径对不对；重新生成工程 |
| `建表失败：... no such table` | 语句顺序或分号问题 | 看 `出错语句` 提示的原文，检查那条 SQL |
| `表：(...)` 里没有表 | 命中"已有数据库"分支但库是空的 | 删掉 `debug/data/app.db` 重跑 |
| 编译报 `C1083: QSqlDatabase` | `.pro` 少 `QT += sql` | 加上后重新生成工程 |

### 4.6 提交 Day 4

```bash
git add .
git commit -m "feat(data): DbManager 完成自动建库与外键配置"
git push
```

---

## Day 5 · 主窗口骨架 + 通知全员

### 5.1 用 Designer 摆界面

打开 `ui/mainwindow.ui`（双击），做三件事：

1. 把默认的 central widget 删掉（`objectName` 是 `centralwidget` 的那个）
2. 从左边拖一个 `List Widget` 进来，**`objectName` 改成 `navList`**，`maximumWidth` 设 160
3. 再从 Containers 里拖一个 `Stacked Widget` 进来，**`objectName` 改成 `stackedWidget`**
4. 选中 centralwidget，点上方工具栏的 `水平布局`（让两个控件左右排开）

> `objectName` 必须**一个字不差**：`navList`、`stackedWidget`。下面的代码按这两个名字找控件。
> 不要用中文字符串当 `objectName`。

保存，回 VS：`扩展 → Qt VS Tools → Open Qt Project File (.pro)` 重新生成一次（`.ui` 改了必须做）。

### 5.2 `ui/mainwindow.h`（完整文件）

```cpp
#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void buildUi();        // 建导航项、放页面
    void connectBus();     // 全项目唯一 connect 的地方（第 2 周填满）

    QWidget* makePlaceholder(const QString& text);   // 占位页，第 2 周会被真页面替换

    Ui::MainWindow* ui;
};
```

### 5.3 `ui/mainwindow.cpp`（完整文件，第 0 周版本）

```cpp
#include "ui/mainwindow.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

#include "data/db_manager.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(QStringLiteral("二手交易平台"));
    resize(1100, 700);

    buildUi();
    connectBus();

    statusBar()->showMessage(QStringLiteral("数据库：") + DbManager::dbPath());
}

MainWindow::~MainWindow()
{
    delete ui;
}

QWidget* MainWindow::makePlaceholder(const QString& text)
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* label = new QLabel(QStringLiteral("【%1】\n（待实现）").arg(text), page);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    return page;
}

void MainWindow::buildUi()
{
    // 五个页面，顺序就是导航顺序。第 2 周把 makePlaceholder 换成真页面。
    ui->stackedWidget->addWidget(makePlaceholder(QStringLiteral("商品检索")));
    ui->stackedWidget->addWidget(makePlaceholder(QStringLiteral("我的商品")));
    ui->stackedWidget->addWidget(makePlaceholder(QStringLiteral("我的订单")));
    ui->stackedWidget->addWidget(makePlaceholder(QStringLiteral("交易记录")));
    ui->stackedWidget->addWidget(makePlaceholder(QStringLiteral("批量导入")));

    ui->navList->addItem(QStringLiteral("商品检索"));
    ui->navList->addItem(QStringLiteral("我的商品"));
    ui->navList->addItem(QStringLiteral("我的订单"));
    ui->navList->addItem(QStringLiteral("交易记录"));
    ui->navList->addItem(QStringLiteral("批量导入"));

    ui->navList->setCurrentRow(0);
    ui->stackedWidget->setCurrentIndex(0);

    connect(ui->navList, &QListWidget::currentRowChanged,
            ui->stackedWidget, &QStackedWidget::setCurrentIndex);
}

void MainWindow::connectBus()
{
    // 第 0 周还没有别的对象，留空。第 2 周在这里接服务与页面。
}
```

### 5.4 验收：第 0 周完成标志

- [ ] 点左边五个导航，右边切换页面，每页显示"（待实现）"
- [ ] 状态栏显示数据库绝对路径
- [ ] `debug/data/app.db` 存在，里面 5 张表 + 8 条分类
- [ ] 输出窗口打印表名和分类条数
- [ ] `git clone` 到一个全新目录，重新打开 `.pro` 转换后 F5 **也能跑**（这条最重要，说明工程是自洽的）

### 5.5 推送并通知

你现在应该在 `feature/D-base` 上（Day 1.4 末尾切过去的）。先确认一下：

```bash
git branch --show-current     # 应该打印 feature/D-base
git status                    # 工作区干净再往下走
git push                      # 推自己的分支，没人拦
```

`main` 和 `develop` 已经开了保护，**不能直接 push**，要走 PR：

1. 打开仓库网页，会看到黄色的 `Compare & pull request` 按钮，点它；
2. 确认页面顶部两个下拉框是 **`base: develop` ← `compare: feature/D-base`**（写反了 PR 就合并到错的地方）；
3. 标题写 `feat: 第 0 周骨架（目录/.pro/DbManager/主窗口）`，点 `Create pull request`；
4. 右侧 `Reviewers` 里点一个（A/B/C 谁都行），微信喊他"看下我的 PR，点 Approve"；
5. 他点完 `Approve`，你点 `Merge pull request` → `Confirm merge`；
6. 切回本地同步：

```bash
git checkout develop
git pull
```

> ⚠️ 找不到人 approve 的时候**不要**去关分支保护。保护的意义就是逼你们互相看一眼代码，
> 答辩时"我们的 develop 分支所有代码都经过 review"这句话是有截图的。

然后在群里发：

> 骨架已推 develop：目录、.pro、四个 .pri、constants.h、opresult.h、001_init.sql、DbManager、主窗口（五个占位页）。
> 你们可以 `git checkout -b feature/xxx` 开工了。
> 改文件前先看 `docs/01-契约.md` 的目录归属表，`ui/mainwindow.*` 归我，别动。

### 5.6 建另外三个人的分支（你在网页上建好更省事）

`Branches → New branch`，分别建 `feature/A-product`、`feature/B-order`、`feature/C-trade`，源分支都选 **`develop`**。

（`feature/D-base` 你 Day 1 就建好了，不用再建。）

建完在群里把三条命令贴给他们，省得每个人问一遍：

```bash
git fetch
git checkout -b feature/A-product origin/develop     # B/C 换成自己的分支名
```

---

# 第二部分 · 第 2 周（接入三个人的页面）

> 前提：A/B/C 的页面类已经能编译（哪怕只有空窗口）。
> **你和他们约定一个最小接口**，写进契约：
>
> ```cpp
> class SearchPage : public QWidget {
>     Q_OBJECT
> public:
>     explicit SearchPage(QWidget* parent = nullptr);
> public slots:
>     void refresh();          // 数据变化时被调用，五个人都有自己的 refresh()
> signals:
>     void buyRequested(int productId);   // 只有检索页有
> };
> ```

## 第 6 步 · `buildServices()`：全项目唯一 new 对象的地方

在 `mainwindow.h` 里加成员：

```cpp
class IUserRepository;   class IProductRepository;
class IOrderRepository;  class ITradeLogRepository;
class AuthService;  class ProductService;  class OrderService;  class TradeService;
class SearchPage;   class MyProductsPage;  class MyOrdersPage;
class TradeLogPage; class ImportPage;
```

```cpp
private:
    // 四个 DAO / 四个 Service，全部由 MainWindow 持有
    IUserRepository*    m_userRepo    = nullptr;
    IProductRepository* m_productRepo = nullptr;
    IOrderRepository*   m_orderRepo   = nullptr;
    ITradeLogRepository* m_logRepo    = nullptr;

    AuthService*    m_authService    = nullptr;
    ProductService* m_productService = nullptr;
    OrderService*   m_orderService   = nullptr;
    TradeService*   m_tradeService   = nullptr;

    // 五个页面
    SearchPage*     m_searchPage     = nullptr;
    MyProductsPage* m_myProductsPage = nullptr;
    MyOrdersPage*   m_myOrdersPage   = nullptr;
    TradeLogPage*   m_tradeLogPage   = nullptr;
    ImportPage*     m_importPage     = nullptr;
```

```cpp
#include "data/user_dao.h"
#include "data/product_dao.h"
#include "data/order_dao.h"
#include "data/trade_log_dao.h"
#include "services/auth_service.h"
#include "services/product_service.h"
#include "services/order_service.h"
#include "services/trade_service.h"

void MainWindow::buildServices()
{
    // 顺序不能错：先 DAO，再 Service（Service 构造要拿到 DAO 指针）
    m_userRepo    = new UserDao(this);
    m_productRepo = new ProductDao(this);
    m_orderRepo   = new OrderDao(this);
    m_logRepo     = new TradeLogDao(this);

    m_authService    = new AuthService(m_userRepo, this);
    m_productService = new ProductService(m_productRepo, this);
    m_orderService   = new OrderService(m_orderRepo, m_productRepo, m_logRepo, this);
    m_tradeService   = new TradeService(m_orderRepo, m_productRepo, m_logRepo, this);
}
```

> 构造参数顺序必须和 `docs/01-契约.md` §7.2 一致，写错了编译就会报参数不匹配。
> **整个项目只有这个函数里出现 `new XxxDao` / `new XxxService`**，别人都不许 `new`。

## 第 7 步 · 把占位页换成真页面

`buildUi()` 里改两处：先建页面对象，再 addWidget。

```cpp
void MainWindow::buildUi()
{
    // 1) 造页面
    m_searchPage     = new SearchPage(this);
    m_myProductsPage = new MyProductsPage(this);
    m_myOrdersPage   = new MyOrdersPage(this);
    m_tradeLogPage   = new TradeLogPage(this);
    m_importPage     = new ImportPage(m_productService, this);

    // 2) 按导航顺序放进 QStackedWidget（顺序必须和 addItem 一致！）
    ui->stackedWidget->addWidget(m_searchPage);
    ui->stackedWidget->addWidget(m_myProductsPage);
    ui->stackedWidget->addWidget(m_myOrdersPage);
    ui->stackedWidget->addWidget(m_tradeLogPage);
    ui->stackedWidget->addWidget(m_importPage);

    ui->navList->addItem(QStringLiteral("商品检索"));
    ui->navList->addItem(QStringLiteral("我的商品"));
    ui->navList->addItem(QStringLiteral("我的订单"));
    ui->navList->addItem(QStringLiteral("交易记录"));
    ui->navList->addItem(QStringLiteral("批量导入"));

    ui->navList->setCurrentRow(0);
    ui->stackedWidget->setCurrentIndex(0);

    connect(ui->navList, &QListWidget::currentRowChanged,
            ui->stackedWidget, &QStackedWidget::setCurrentIndex);
}
```

> ⚠️ **`addWidget` 的顺序必须和 `addItem` 的顺序完全一致**，否则点"交易记录"出来的是"我的订单"。
> 这是最容易犯又最难一眼看出的错。

## 第 8 步 · `connectBus()`：全项目唯一 connect 的地方

```cpp
void MainWindow::connectBus()
{
    // 页面 → 主窗口：检索页双击商品，交给主窗口去弹购买框
    connect(m_searchPage, &SearchPage::buyRequested,
            this, &MainWindow::onBuyRequested);

    // 服务 → 主窗口：任何数据变了，让三个列表页各重查一次
    connect(m_productService, &ProductService::productsChanged,
            this, &MainWindow::onDataChanged);
    connect(m_tradeService, &TradeService::orderStatusChanged,
            this, &MainWindow::onDataChanged);
    connect(m_orderService, &OrderService::orderCreated,
            this, &MainWindow::onDataChanged);

    // 登录状态变化
    connect(m_authService, &AuthService::loginSucceeded,
            this, &MainWindow::onLoginChanged);
    connect(m_authService, &AuthService::logoutDone,
            this, &MainWindow::onLoginChanged);
}

void MainWindow::onDataChanged()
{
    // 不做增量刷新：谁的数据可能变了就整页重查一次，几十条数据无感
    if (m_searchPage)     m_searchPage->refresh();
    if (m_myProductsPage) m_myProductsPage->refresh();
    if (m_myOrdersPage)   m_myOrdersPage->refresh();
}

void MainWindow::onBuyRequested(int productId)
{
    if (!Session::instance().isLoggedIn()) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先登录"));
        return;
    }
    BuyDialog dlg(productId, this);      // B 的对话框
    if (dlg.exec() == QDialog::Accepted)
        statusBar()->showMessage(QStringLiteral("下单成功"), 3000);
}
```

## 第 9 步 · 登录流程接进 `main.cpp`

```cpp
#include "core/session.h"
#include "services/auth_service.h"
#include "data/user_dao.h"
#include "ui/pages/auth/login_dialog.h"
```

```cpp
    // 3) 先弹登录框；用户点 X 直接退出，不要进到一个没登录的主界面
    UserDao userDao;
    AuthService authService(&userDao);
    LoginDialog loginDlg(&authService);
    if (loginDlg.exec() != QDialog::Accepted)
        return 0;

    MainWindow w;
    w.setCurrentUser(Session::instance().currentUser());   // 标题栏显示 alice
    w.show();
```

> 这里 `UserDao` 和 `AuthService` 是**栈对象**，天生在 `main` 结束前销毁，不需要 `delete`。
> 主窗口里那一套是 `new` 出来的（因为要挂在 `QObject` 树上），两处并存不影响——`DbManager::db()` 是单例，谁用都是同一条连接。

## 第 10 步 · 演示数据（`工具 → 重置演示数据`）

在 `mainwindow.ui` 里加菜单：`工具 → 重置演示数据`，槽名 `onResetDemoData()`。

```cpp
void MainWindow::onResetDemoData()
{
    if (QMessageBox::question(this, QStringLiteral("重置演示数据"),
            QStringLiteral("会补充演示账号、商品和一单示例订单，已有的不会重复。继续？"))
        != QMessageBox::Yes)
        return;

    // ---------- 1) 三个用户：走 UserDao::insert，明文密码由 DAO 内部加盐哈希 ----------
    const QVector<QPair<QString, QString>> seedUsers = {
        { QStringLiteral("alice"), QStringLiteral("Alice") },
        { QStringLiteral("bob"),   QStringLiteral("Bob")   },
        { QStringLiteral("carol"), QStringLiteral("Carol") },
    };

    QHash<QString, int> uid;
    for (const auto& u : seedUsers) {
        int id = -1;
        if (!m_userRepo->exists(u.first)) {
            User user;
            user.username     = u.first;
            user.nickname     = u.second;
            user.passwordHash = QStringLiteral("123456");   // 明文！DAO 会哈希
            auto r = m_userRepo->insert(user);
            if (!r.ok) {
                QMessageBox::warning(this, QStringLiteral("重置失败"), r.message);
                return;
            }
            id = user.id;
        } else {
            auto existing = m_userRepo->findByUsername(u.first);
            id = existing ? existing->id : -1;
        }
        uid[u.first] = id;
    }

    if (uid["alice"] <= 0 || uid["bob"] <= 0) {
        QMessageBox::warning(this, QStringLiteral("重置失败"),
                             QStringLiteral("演示账号创建失败"));
        return;
    }

    // ---------- 2) 12 件商品：直接 SQL，seller_id 用真实 id ----------
    struct Seed { const char* title; int cat; double price; int cond; const char* desc; };
    const QVector<Seed> seedProducts = {
        { "九成新 iPhone 13 128G",  6, 3200.0, 9, "国行无划痕，电池健康 92%" },
        { "Redmi K60 12+256",       6, 1450.0, 8, "自用一年，配件齐全" },
        { "二手 ThinkPad T480",     7, 2100.0, 7, "i5-8250U 16G 512G" },
        { "罗技 K380 键盘",         7,   89.0, 8, "蓝牙连接正常" },
        { "AirPods 3 港版",         8,  620.0, 8, "无明显划痕" },
        { "漫步者蓝牙耳机",          8,   85.0, 6, "音质正常，耳罩有磨损" },
        { "高等数学 第七版 上下册",   2,   25.0, 7, "有笔记，无缺页" },
        { "C++ Primer 第 5 版",      2,   45.0, 8, "几乎全新" },
        { "大学物理 教材",            2,   18.0, 6, "封面有折痕" },
        { "宜家 小书桌",              3,  120.0, 7, "自提，需自己拆装" },
        { "优衣库 羽绒服 L 码",        4,  150.0, 8, "穿过两次" },
        { "迪卡侬 瑜伽垫",            5,   35.0, 7, "用过几次，已清洁" },
    };

    QSqlDatabase& db = DbManager::db();
    if (!db.transaction()) {
        QMessageBox::warning(this, QStringLiteral("重置失败"), QStringLiteral("开启事务失败"));
        return;
    }

    int inserted = 0;
    const QStringList sellers = { "alice", "bob" };
    for (int i = 0; i < seedProducts.size(); ++i) {
        const Seed& s = seedProducts[i];
        const int sellerId = uid.value(sellers[i % sellers.size()]);

        // 用标题判重，可反复点"重置"
        QSqlQuery chk(db);
        chk.prepare("SELECT COUNT(*) FROM products WHERE title = ?");
        chk.addBindValue(QString::fromUtf8(s.title));
        chk.exec();
        if (chk.next() && chk.value(0).toInt() > 0)
            continue;

        QSqlQuery q(db);
        q.prepare("INSERT INTO products(seller_id, category_id, title, description, "
                  "price, condition_level, status) VALUES(?,?,?,?,?,?,?)");
        q.addBindValue(sellerId);
        q.addBindValue(s.cat);
        q.addBindValue(QString::fromUtf8(s.title));
        q.addBindValue(QString::fromUtf8(s.desc));
        q.addBindValue(s.price);
        q.addBindValue(s.cond);
        q.addBindValue(Const::ProductStatus::OnSale);
        if (!q.exec()) {
            db.rollback();
            QMessageBox::warning(this, QStringLiteral("重置失败"), q.lastError().text());
            return;
        }
        ++inserted;
    }

    // ---------- 3) 一单进行中的订单：bob 买了 alice 的一件（状态 PAID，带 2 条日志） ----------
    int    demoProductId = -1;
    double demoPrice     = 0.0;
    {
        QSqlQuery q(db);
        q.exec(QStringLiteral("SELECT id, price FROM products "
                              "WHERE status = 'ON_SALE' AND seller_id = %1 "
                              "ORDER BY id LIMIT 1").arg(uid.value("alice")));
        if (q.next()) {
            demoProductId = q.value(0).toInt();
            demoPrice     = q.value(1).toDouble();   // 金额取商品真实价格，别写死
        }
    }

    if (demoProductId > 0 && uid.value("bob") > 0) {
        QSqlQuery exists(db);
        exists.exec("SELECT COUNT(*) FROM orders");
        const bool hasOrder = exists.next() && exists.value(0).toInt() > 0;

        if (!hasOrder) {
            // 3.1 把商品标成已售出
            QSqlQuery up(db);
            up.prepare("UPDATE products SET status = 'SOLD' WHERE id = ?");
            up.addBindValue(demoProductId);
            up.exec();

            // 3.2 建订单
            QSqlQuery ins(db);
            ins.prepare("INSERT INTO orders(order_no, product_id, buyer_id, seller_id, "
                        "amount, status, remark) VALUES(?,?,?,?,?,?,?)");
            ins.addBindValue(QDate::currentDate().toString("yyyyMMdd") + "-0001");
            ins.addBindValue(demoProductId);
            ins.addBindValue(uid.value("bob"));
            ins.addBindValue(uid.value("alice"));
            ins.addBindValue(demoPrice);
            ins.addBindValue(Const::OrderStatus::Paid);
            ins.addBindValue(QStringLiteral("演示订单"));
            if (!ins.exec()) {
                db.rollback();
                QMessageBox::warning(this, QStringLiteral("重置失败"), ins.lastError().text());
                return;
            }
            const int orderId = ins.lastInsertId().toInt();

            // 3.3 两条日志：下单、付款
            const QVector<QPair<QString, QString>> steps = {
                { QString(),                    Const::OrderStatus::PendingPay },
                { Const::OrderStatus::PendingPay, Const::OrderStatus::Paid     },
            };
            const QVector<QString> actions = { Const::TradeAction::Create, Const::TradeAction::Pay };
            const QVector<int>  operators  = { uid.value("bob"), uid.value("bob") };

            for (int i = 0; i < steps.size(); ++i) {
                QSqlQuery log(db);
                log.prepare("INSERT INTO trade_log(order_id, from_status, to_status, "
                            "action, operator_id, remark) VALUES(?,?,?,?,?,?)");
                log.addBindValue(orderId);
                log.addBindValue(steps[i].first);
                log.addBindValue(steps[i].second);
                log.addBindValue(actions[i]);
                log.addBindValue(operators[i]);
                log.addBindValue(i == 0 ? QStringLiteral("下单") : QStringLiteral("付款"));
                if (!log.exec()) {
                    db.rollback();
                    QMessageBox::warning(this, QStringLiteral("重置失败"), log.lastError().text());
                    return;
                }
            }
        }
    }

    if (!db.commit()) {
        db.rollback();
        QMessageBox::warning(this, QStringLiteral("重置失败"), QStringLiteral("提交失败"));
        return;
    }

    onDataChanged();
    QMessageBox::information(this, QStringLiteral("完成"),
                             QStringLiteral("新增商品 %1 件。记忆账号：alice / bob / carol，"
                                            "密码都是 123456").arg(inserted));
}
```

`mainwindow.cpp` 顶部把这一组 include 补齐（少一个就是一堆 C2065/C2027）：

```cpp
#include "core/common/constants.h"
#include "core/session.h"
#include "core/entity/user.h"
#include "data/db_manager.h"
#include "data/user_dao.h"
#include "services/auth_service.h"
#include "ui/pages/auth/login_dialog.h"
#include "ui/pages/order/buy_dialog.h"
#include "ui/pages/product/search_page.h"
#include "ui/pages/product/my_products_page.h"
#include "ui/pages/order/my_orders_page.h"
#include "ui/pages/trade/trade_log_page.h"
#include "ui/pages/import/import_page.h"
#include <QDate>
#include <QHash>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
```

> 演示订单只建一次（先查 `orders` 有没有数据），所以这个菜单点十次也不会变成十单。

## 第 11 步 · `ImportService`

### 11.1 `services/import_service.h`（完整文件）

```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class IProductRepository;
class IUserRepository;

struct ImportReport
{
    int         total     = 0;
    int         succeeded = 0;
    QStringList errors;          // "第 3 行：价格不是有效数字"
};

// CSV 批量导入：只要有 1 行不合法，一条都不入库
class ImportService : public QObject
{
    Q_OBJECT
public:
    ImportService(IProductRepository* productRepo, IUserRepository* userRepo,
                  QObject* parent = nullptr);

    ImportReport importCsv(const QString& filePath, int defaultSellerId);

private:
    IProductRepository* m_productRepo;
    IUserRepository*    m_userRepo;
};
```

### 11.2 `services/import_service.cpp`（完整文件）

```cpp
#include "services/import_service.h"

#include <QCoreApplication>
#include <QFile>
#include <QMap>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringConverter>
#include <QStringDecoder>
#include <QVariant>

#include "core/common/constants.h"
#include "core/entity/product.h"
#include "core/repository/i_product_repository.h"
#include "data/db_manager.h"

namespace {

// 把一个 CSV 行切成字段，处理引号和 "" 转义
QStringList parseCsvLine(const QString& line)
{
    QStringList out;
    QString field;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line.at(i);
        if (inQuotes) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
                    field += QLatin1Char('"');    // "" → 一个引号
                    ++i;
                } else {
                    inQuotes = false;             // 引号结束
                }
            } else {
                field += c;
            }
        } else {
            if (c == QLatin1Char('"')) {
                inQuotes = true;
            } else if (c == QLatin1Char(',')) {
                out << field;
                field.clear();
            } else {
                field += c;
            }
        }
    }
    out << field;                                  // 最后一个字段
    return out;
}

// 先按 UTF-8 解，失败再按系统编码（中文 Windows = GBK）解
QString decodeBytes(const QByteArray& bytes)
{
    QStringDecoder utf8(QStringConverter::Utf8);
    QString text = utf8.decode(bytes);
    if (utf8.hasError()) {
        QStringDecoder sys(QStringConverter::System);
        text = sys.decode(bytes);
    }
    if (text.startsWith(QChar(0xFEFF)))            // 去掉 BOM
        text.remove(0, 1);
    return text;
}

// 分类名（支持"数码电子/手机"这种父/子写法）→ id，找不到返回 -1
int findCategoryId(QSqlDatabase& db, const QString& name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
        return -1;

    const int slash = trimmed.indexOf(QLatin1Char('/'));
    const QString child  = slash >= 0 ? trimmed.mid(slash + 1).trimmed() : trimmed;
    const QString parent = slash >= 0 ? trimmed.left(slash).trimmed() : QString();

    QSqlQuery q(db);
    if (parent.isEmpty()) {
        q.prepare("SELECT id FROM categories WHERE name = ?");
        q.addBindValue(child);
    } else {
        q.prepare("SELECT c.id FROM categories c "
                  "JOIN categories p ON p.id = c.parent_id "
                  "WHERE c.name = ? AND p.name = ?");
        q.addBindValue(child);
        q.addBindValue(parent);
    }
    if (q.exec() && q.next())
        return q.value(0).toInt();
    return -1;
}

} // namespace

ImportService::ImportService(IProductRepository* productRepo, IUserRepository* userRepo,
                             QObject* parent)
    : QObject(parent)
    , m_productRepo(productRepo)
    , m_userRepo(userRepo)
{
}

ImportReport ImportService::importCsv(const QString& filePath, int defaultSellerId)
{
    ImportReport report;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        report.errors << QStringLiteral("无法打开文件：%1").arg(filePath);
        return report;
    }
    const QString text = decodeBytes(f.readAll());
    f.close();

    QStringList lines = text.split(QRegularExpression(QStringLiteral("\r\n|\n|\r")),
                                   Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        report.errors << QStringLiteral("文件是空的");
        return report;
    }

    // 第一行必须是表头
    const QStringList header = parseCsvLine(lines.first());
    QStringList expect = { "title", "category", "price", "condition_level", "description" };
    QStringList actual;
    for (const QString& h : header)
        actual << h.trimmed().toLower();
    if (actual.size() < 4 || actual.mid(0, 4) != expect.mid(0, 4)) {
        report.errors << QStringLiteral("表头不对，应该以 "
                                        "title,category,price,condition_level 开头，实际是：%1")
                             .arg(lines.first());
        return report;
    }
    const bool hasDescription = actual.size() >= 5 && actual.at(4) == QStringLiteral("description");

    // 逐行校验，全部先攒起来，不落库
    QVector<Product> rows;
    QSqlDatabase& db = DbManager::db();

    for (int i = 1; i < lines.size(); ++i) {
        const int lineNo = i + 1;
        ++report.total;
        const QStringList cols = parseCsvLine(lines.at(i));

        if (cols.size() < 4) {
            report.errors << QStringLiteral("第 %1 行：列数不足（至少 4 列）").arg(lineNo);
            continue;
        }

        const QString title = cols.at(0).trimmed();
        const QString cat   = cols.at(1).trimmed();
        const QString priceStr = cols.at(2).trimmed();
        const QString condStr  = cols.at(3).trimmed();
        const QString desc  = hasDescription && cols.size() >= 5 ? cols.at(4).trimmed() : QString();

        if (title.isEmpty()) {
            report.errors << QStringLiteral("第 %1 行：标题不能为空").arg(lineNo);
            continue;
        }
        if (title.size() > 60) {
            report.errors << QStringLiteral("第 %1 行：标题超过 60 字").arg(lineNo);
            continue;
        }

        bool priceOk = false;
        const double price = priceStr.toDouble(&priceOk);
        if (!priceOk || price <= 0) {
            report.errors << QStringLiteral("第 %1 行：价格不是有效数字（%2）").arg(lineNo).arg(priceStr);
            continue;
        }

        int cond = 9;
        if (!condStr.isEmpty()) {
            bool condOk = false;
            cond = condStr.toInt(&condOk);
            if (!condOk || cond < 1 || cond > 10) {
                report.errors << QStringLiteral("第 %1 行：成色必须是 1~10 的整数").arg(lineNo);
                continue;
            }
        }

        const int catId = findCategoryId(db, cat);
        if (catId <= 0) {
            report.errors << QStringLiteral("第 %1 行：分类\"%2\"不存在").arg(lineNo).arg(cat);
            continue;
        }

        Product p;
        p.sellerId       = defaultSellerId;
        p.categoryId     = catId;
        p.title          = title;
        p.description    = desc;
        p.price          = price;
        p.conditionLevel = cond;
        rows << p;
    }

    if (!report.errors.isEmpty()) {
        report.succeeded = 0;
        return report;                     // 有错就一条不导
    }

    // 全部合法 → 整批一个事务
    if (!db.transaction()) {
        report.errors << QStringLiteral("开启事务失败");
        return report;
    }
    for (int i = 0; i < rows.size(); ++i) {
        auto r = m_productRepo->insert(rows[i]);
        if (!r.ok) {
            db.rollback();
            report.errors << QStringLiteral("第 %1 条写入失败：%2").arg(i + 1).arg(r.message);
            report.succeeded = 0;
            return report;
        }
    }
    if (!db.commit()) {
        db.rollback();
        report.errors << QStringLiteral("提交事务失败");
        report.succeeded = 0;
        return report;
    }

    report.succeeded = rows.size();
    return report;
}
```

> 别忘了 `#include <QRegularExpression>`（上面用了却没列在 include 里，加上）。

### 11.3 `ui/pages/import/import_page.h`（完整文件）

```cpp
#pragma once

#include <QWidget>

class ImportService;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

class ImportPage : public QWidget
{
    Q_OBJECT
public:
    explicit ImportPage(ImportService* service, QWidget* parent = nullptr);

public slots:
    void refresh() {}                 // 导入页没有列表要刷新，留空

private slots:
    void onBrowse();
    void onImport();

private:
    ImportService*  m_service = nullptr;
    QLineEdit*      m_pathEdit = nullptr;
    QPushButton*    m_browseBtn = nullptr;
    QPushButton*    m_importBtn = nullptr;
    QPlainTextEdit* m_resultEdit = nullptr;
};
```

### 11.4 `ui/pages/import/import_page.cpp`（完整文件）

```cpp
#include "ui/pages/import/import_page.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "core/session.h"
#include "services/import_service.h"

ImportPage::ImportPage(ImportService* service, QWidget* parent)
    : QWidget(parent)
    , m_service(service)
{
    auto* root = new QVBoxLayout(this);

    auto* tip = new QLabel(QStringLiteral(
        "CSV 第一行必须是表头：title,category,price,condition_level,description\n"
        "卖家就是当前登录用户。任何一行出错，整批都不会导入。"), this);
    root->addWidget(tip);

    auto* row = new QHBoxLayout();
    m_pathEdit  = new QLineEdit(this);
    m_pathEdit->setReadOnly(true);
    m_pathEdit->setPlaceholderText(QStringLiteral("还没有选择文件"));
    m_browseBtn = new QPushButton(QStringLiteral("选择 CSV 文件…"), this);
    m_importBtn = new QPushButton(QStringLiteral("开始导入"), this);
    m_importBtn->setEnabled(false);
    row->addWidget(m_pathEdit, 1);
    row->addWidget(m_browseBtn);
    row->addWidget(m_importBtn);
    root->addLayout(row);

    m_resultEdit = new QPlainTextEdit(this);
    m_resultEdit->setReadOnly(true);
    root->addWidget(m_resultEdit, 1);

    connect(m_browseBtn, &QPushButton::clicked, this, &ImportPage::onBrowse);
    connect(m_importBtn, &QPushButton::clicked, this, &ImportPage::onImport);
}

void ImportPage::onBrowse()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择 CSV 文件"), QString(),
        QStringLiteral("CSV 文件 (*.csv);;所有文件 (*.*)"));
    if (path.isEmpty())
        return;

    m_pathEdit->setText(path);
    m_importBtn->setEnabled(true);
    m_resultEdit->setPlainText(QStringLiteral("已选择：") + path);
}

void ImportPage::onImport()
{
    if (!Session::instance().isLoggedIn()) {
        m_resultEdit->setPlainText(QStringLiteral("请先登录再导入"));
        return;
    }
    if (!m_service) {
        m_resultEdit->setPlainText(QStringLiteral("导入服务不可用"));
        return;
    }

    const ImportReport r = m_service->importCsv(m_pathEdit->text(),
                                                Session::instance().currentUser().id);

    QString out = QStringLiteral("共 %1 行，成功 %2 行\n").arg(r.total).arg(r.succeeded);
    if (!r.errors.isEmpty()) {
        out += QStringLiteral("\n错误：\n");
        for (const QString& e : r.errors)
            out += QStringLiteral("· ") + e + QLatin1Char('\n');
    }
    m_resultEdit->setPlainText(out);
}
```

## 第 12 步 · 第 2 周收尾

- [ ] 五个页面都能点开、都不崩
- [ ] 重置演示数据 → 三个账号能登录、12 件商品能搜到、有一单"已付款"的订单和 2 条日志
- [ ] 导入一个 10 行 CSV → 成功 10 行；把某行价格改成 `abc` → 成功 0 行并指出是第几行
- [ ] 导航顺序和页面顺序一致（点"批量导入"出来的是导入页）

---

# 第三部分 · 第 3~4 周（打包与收尾）

## 第 13 步 · 发布版打包

```bash
cd /d/DSH-workspace/Second-handTradingPlatformDev/Second-handTradingPlatform
# 1) 在 VS 里把配置切到 Release x64，Ctrl+Shift+B
# 2) 打包
/d/Qt6/6.11.2/msvc2022_64/bin/windeployqt.exe --release --no-translations \
    --compiler-runtime release/Second-handTradingPlatform.exe
```

（`--compiler-runtime` 会把 MSVC 运行库一起带上，别人电脑就不用装 redist 了。）

**逐项确认** `release/` 里必须有：

| 文件/目录 | 缺了会怎样 |
|---|---|
| `Second-handTradingPlatform.exe` | — |
| `Qt6Core.dll`、`Qt6Gui.dll`、`Qt6Widgets.dll`、`Qt6Sql.dll` | 双击直接报"找不到 Qt6Core.dll" |
| `sqldrivers\qsqlite.dll` | 报"driver not loaded"，库打不开 |
| `platforms\qwindows.dll` | 报"could not find or load the Qt platform plugin windows" |
| `vc_redist.x64.exe` 或 msvcp140.dll 等 | 报"缺少 MSVCP140.dll" |

## 第 14 步 · 换台电脑验证（必须做）

1. 把整个 `release/` 目录（含 `data/`）拷到 U 盘
2. 找一台**没装过 Qt** 的电脑，拷到桌面，双击 exe
3. 依次验证：能开 → 能登录（alice/123456）→ 能搜到商品 → 能下单 → 能改状态 → 能导入 CSV
4. 注意：不删除 `data/app.db`，数据要跟着走；想给对方一个干净的，就先删 `data/` 再拷

## 第 15 步 · 答辩前一天的自检

```bash
git checkout develop && git pull
git shortlog -sne                  # 四个人的提交数应该都不难看
git log --oneline --all --graph    # 分支演进图（截图备用）
```

- [ ] `debug/`、`release/`、`data/`、`ui_*.h` 都**没有**被提交
- [ ] `main` = `develop` 的最终状态，能编译
- [ ] 演示数据能一键重置，重置后演示流程连续走三遍不出错
- [ ] 打包版在别人电脑上跑通过
- [ ] `docs/meetings/` 每份记录都有日期和责任人

---

# 附 · D 的常见坑速查

| 现象 | 原因 | 解决 |
|---|---|---|
| `C1083: 无法打开包括文件 "core/entity/product.h"` | `INCLUDEPATH` 没加或新增文件没进工程 | `.pro` 里 `INCLUDEPATH += $$PWD`；`Qt VS Tools → Open Qt Project File (.pro)` 重新生成 |
| 新增 `.cpp` 后 `LNK2019` | 新文件没被收进工程 | 重新生成工程（`.pri` 通配符需要 qmake 重跑才生效） |
| 改了 `.ui` 界面没反应 | `ui_*.h` 是生成物 | 重新生成工程 |
| `vtable for XXX` 未定义 | 声明了 `Q_OBJECT` 但没被 moc | 确认头文件在 `HEADERS` 里（通配符会收），重新生成工程 |
| 中文乱码 | 源文件编码 | 全部另存为 **UTF-8 无 BOM**；`.pro` 里 `QMAKE_CXXFLAGS += /utf-8` |
| 数据库路径找不到 | 用了相对路径 | 一律 `QCoreApplication::applicationDirPath()` |
| 外键约束不生效 | `PRAGMA foreign_keys` 是**按连接**的 | 每个新打开的连接都要执行一次 |
| 建表报 `near ";"` 之类 | SQL 脚本里有行内注释或多写了分号 | 按 Day 3.3 的三条规矩改 |
| `git push` 被拒 | 分支保护要求 PR | 走 PR 流程：开分支 → 推 → 网页建 PR → 找人 approve |
| 两个页面对不上 | `addWidget` 与 `addItem` 顺序不一致 | 两边顺序都按：检索 / 我的商品 / 我的订单 / 交易记录 / 批量导入 |
