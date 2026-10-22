# D · 详细施工步骤（每一步都讲清楚）

> 这是 D 一个人的施工手册：从空文件夹做到能双击运行的程序。
> 这一版给每一步、每一段代码都补了说明，包括这一步在做什么、为什么非这样不可、代码里那些名字是怎么起的。
>
> 三份文件按顺序读：

| 文件 | 内容 |
| --- | --- |
| **本文件** | 第 0 周：建仓库、目录骨架、数据库、空主窗口 |
| [D-施工步骤-第2周.md](D-施工步骤-第2周.md) | 第 2 周：接入 A/B/C 的页面、演示数据、CSV 导入 |
| [D-施工步骤-第3周.md](D-施工步骤-第3周.md) | 第 3~4 周：打包、换机验证、答辩自检 |

> 约定：路径都相对 `Second-handTradingPlatform/`（`.pro` 所在的那个目录）；命令在 Git Bash 里执行，路径写成 `/d/...` 这种风格。

---

# 第 0 章 · 先把这些名字认全

代码读不懂，八成不是逻辑难，是名字陌生。这一章把整个项目里会反复出现的命名规律一次讲完，后面每一步就不用再解释了。

## 0.1 仓库里有两个名字很像的目录

```
D:\DSH-workspace\
└── Second-handTradingPlatformDev\        ← Git 仓库根，README 和 docs 在这一层
    ├── README.md
    ├── docs\                             ← 所有文档
    └── Second-handTradingPlatform\       ← Qt 工程本体，.pro 和 main.cpp 在这一层
        ├── Second-handTradingPlatform.pro
        ├── main.cpp
        ├── core\  data\  services\  ui\  sql\
```

`Second-handTradingPlatformDev` 末尾的 `Dev` 是 development 的缩写，表示“这个文件夹装的是整个开发项目”，里面既有代码也有文档。真正的 Qt 工程是里面那个不带 `Dev` 的目录。VS 打开的、qmake 认的、`git clone` 下来要编译的，都是里面那个。

## 0.2 文件名的规律

全小写，单词之间用下划线。

| 文件名 | 拆开看 | 是什么 |
| --- | --- | --- |
| `product_dao.h` | product + DAO | 商品的数据库访问代码 |
| `i_product_repository.h` | I + product + repository | 商品的仓库**接口**，开头那个 `i` 就是 interface |
| `product_service.h` | product + service | 商品的业务逻辑 |
| `search_page.h` | search + page | 检索页面 |
| `buy_dialog.h` | buy + dialog | 购买对话框 |
| `trade_state_machine.h` | trade + state + machine | 交易状态机 |
| `db_manager.h` | db + manager | 数据库管理器 |

`.h` 是头文件，只写声明；`.cpp` 是实现文件，写函数体。一个类对应一对 `.h`/`.cpp`，名字相同。

## 0.3 代码里名字的规律

| 写法 | 含义 | 例子 |
| --- | --- | --- |
| `CamelCase`，首字母大写 | 类名、结构体名 | `ProductDao`、`SearchPage`、`OpResult` |
| `camelCase`，首字母小写 | 函数名、变量名 | `findById()`、`salt`、`lineNo` |
| `m_` 开头 | 类的成员变量（member） | `m_userRepo`、`m_pathEdit` |
| `s_` 开头 | 静态成员变量（static） | 本项目没用到，见到别慌 |
| 全大写 + 下划线 | 宏、状态值字面量 | `ON_SALE`、`QT_DEBUG` |
| `Const::` 开头 | 全项目的常量都在这 | `Const::ProductStatus::OnSale` |
| `I` 开头 | 接口（只声明、不实现） | `IProductRepository` |

`m_` 这个前缀看着多余，作用是让人一眼分清“这是成员变量”还是“这是参数”。比如构造函数里写 `m_userRepo(userRepo)`，左边带 `m_` 的是成员，右边不带的是传进来的参数，同名也不乱。

## 0.4 程序里四层分别叫什么

| 层 | 目录 | 类名后缀 | 职责 | 不许做什么 |
| --- | --- | --- | --- | --- |
| 界面层 | `ui/` | `Page`、`Dialog`、`MainWindow` | 显示、接收点击 | 不许出现 SQL |
| 服务层 | `services/` | `Service`、`StateMachine` | 业务规则、事务 | 不许出现 QWidget |
| 数据层 | `data/` | `Dao`、`Manager` | 执行 SQL | 不许出现 QWidget |
| 接口与模型层 | `core/` | `Repository`、结构体 | 类型定义、接口声明 | 不许依赖上面三层 |

`DAO` 是 Data Access Object 的缩写，意思是“专门负责跟数据库打交道的对象”。`Repository` 意思是“仓库”，在这一层指接口：上层只说“我要查商品”，不关心底下是 SQLite 还是别的数据库。两个人一个写接口、一个写实现，靠的就是这层约定。

## 0.5 Qt 里绕不开的几个名字

| 名字 | 是什么 |
| --- | --- |
| `QString` | Qt 的字符串类型，内部是 Unicode，中文不会乱码 |
| `QStringLiteral("中文")` | 编译期就构造好的 `QString`，比 `QString("中文")` 快，也不会有编码问题 |
| `QVariant` | “什么都能装”的容器，绑定 SQL 参数时用 |
| `QObject` | Qt 所有能挂到对象树、能用信号槽的类的基类 |
| `Q_OBJECT` | 写在类里的宏，告诉 moc 这个类有信号槽，需要额外生成代码 |
| 信号 / 槽 | Qt 的事件通知机制。信号是“事情发生了”，槽是“发生了就调用这个函数” |
| `parent` 参数 | 指向父对象。父对象销毁时子对象自动跟着销毁，所以 `new` 出来的东西基本不用手动 `delete` |
| `qDebug()` | 往输出窗口打印调试信息，相当于 Qt 的 `printf` |

`Q_OBJECT` 这一条最容易踩坑：类里写了 `Q_OBJECT`，就必须让 moc 重新生成代码（VS 里就是重新生成工程），否则编译时会出现 `vtable for XXX` 未定义，看着像玄学，其实就是这个宏没被处理。

## 0.6 构建系统里的名字

| 名字 | 含义 |
| --- | --- |
| `.pro` | qmake 的工程文件，一个工程只有一个，写在根目录 |
| `.pri` | 被 `.pro` `include()` 进来的片段，语法和 `.pro` 一样，用来把一堆文件清单拆到各目录 |
| qmake | Qt 的构建工具，把 `.pro` 翻译成 VS 的工程文件（`.vcxproj`） |
| uic | 把 `.ui`（Designer 存的 XML）翻译成 C++ 头文件 `ui_*.h` |
| moc | 处理带 `Q_OBJECT` 的类，生成 `moc_*.cpp` |
| rcc | 把 `.qrc` 里登记的资源文件编进 exe，生成 `qrc_*.cpp` |
| `ui_*.h`、`moc_*.cpp`、`qrc_*.cpp` | 全是生成物，**不要手改、不要提交**，`.gitignore` 里已经排除了 |
| `$$PWD` | qmake 变量，当前 `.pro` / `.pri` 文件所在目录 |
| `$$files(路径, true)` | qmake 函数，列出匹配的文件，第二个参数 `true` 表示连子目录一起 |
| `DESTDIR` | 编译产物（exe）放哪个目录 |

## 0.7 数据库里的名字

| 名字 | 含义 |
| --- | --- |
| SQLite | 一个文件就是一个数据库，不需要安装服务，Qt 自带驱动 |
| `PRAGMA` | SQLite 的设置命令，比如打开外键、切换日志模式 |
| `PRAGMA foreign_keys = ON` | 打开外键约束。默认是**关**的，而且**每个连接都要设一次** |
| `WAL` | Write-Ahead Logging，一种日志模式。开了之后读写不互相卡住，程序崩了也不容易坏库 |
| `sqlite_master` | SQLite 自带的表，记录这个库里有哪些表、索引 |
| 参数绑定 | 用 `?` 占位再 `addBindValue()` 填值，不要用字符串拼 SQL。拼字符串会被 `'` 搞崩，也有注入风险 |

---

# 第一部分 · 第 0 周（四天出骨架，第五天通知全员）

第 0 周的产出是一个“能开窗、能建库、目录齐全”的空壳。四个人的活都从这个壳上长出来，所以这一周 D 最忙，后面三周 D 最闲。

## Day 1 · 建仓库、分支保护、邀请人

### 1.1 在 GitHub 上建仓库

1. 打开 https://github.com/new
2. `Repository name` 填 `Second-handTradingPlatform`
3. 可见性选 **Public**（见下方说明）
4. **不要**勾 `Add a README file`、**不要**勾 `Add .gitignore`、**不要**选 license，本地已经有这些文件，勾了会产生一个多余的提交，push 时还会冲突
5. 点 `Create repository`，把页面上那个 `https://github.com/你的账号/Second-handTradingPlatform.git` 记下来

> **为什么选 Public**：GitHub 免费账号的私有仓库不支持强制分支保护，规则存下来也不生效，谁都能直接往 `main` 推。课程设计没有保密需求，公开之后分支保护立刻生效，老师也能不加协作者就看到你们的仓库和提交记录。

### 1.2 邀请另外三个人

`仓库页 → Settings → Collaborators → Add people`，输入他们的 GitHub 用户名，三个人都加。

每个人必须用自己的账号。答辩时要打开 Insights 的 Contributors 图证明是四个人做的，共用一个账号那张图就只有一个人。

### 1.3 把本地目录变成 Git 仓库

`Second-handTradingPlatformDev` 现在只是一个普通文件夹，里面没有 `.git`。先确认一下：

```bash
cd /d/DSH-workspace/Second-handTradingPlatformDev
ls -a
```

看不到 `.git` 就对了。`.git` 是 Git 自己建的，不要手动创建，也不要删。

**先建 `.gitignore`**，放在 `Second-handTradingPlatformDev/` 下，和 `README.md`、`docs/` 同级：

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

# ---- 运行时数据（只忽略 exe 旁边生成的那份，别把源码里的 data/ 目录一起忽略了）----
/Second-handTradingPlatform/data/app.db
/Second-handTradingPlatform/data/app.db-journal
/Second-handTradingPlatform/data/app.db-wal
/Second-handTradingPlatform/data/app.db-shm
/Second-handTradingPlatform/data/images/
/Second-handTradingPlatform/logs/
logs/
*.db
*.db-journal
*.db-wal
*.db-shm

# ---- 系统 ----
Thumbs.db
desktop.ini
```

**这个文件在做什么**

`.gitignore` 里写的路径，Git 一律当没看见，不会出现在 `git status` 里，也不会被提交。它的作用是划清“哪些是源码、哪些是本机跑出来的东西”。

分三类：

- `build/`、`debug/`、`release/`、`*.exe`、`*.obj`：编译产物，每台机器编出来都不一样，传上去纯属浪费空间，而且四个人一提交就互相覆盖。
- `ui_*.h`、`moc_*.cpp`、`qrc_*.cpp`：Qt 自动生成的文件，你机器上生成的内容和别人的不一样，谁提交谁让别人天天冲突。这一行是全文最重要的之一。
- `data/`、`*.db`：程序运行时建的数据库，里面有演示数据，不该进仓库。想给别人种子数据就写 SQL，不要传二进制文件。

> ⚠️ **这里有个很容易踩的坑，会毁掉整个工程**：`data/` 这种只写一个目录名的写法是**通配**，它会匹配任何一层叫 `data` 的目录 —— 包括你第 2 步刚要建的**源码目录** `Second-handTradingPlatform/data/`（DAO 全在里面，还有 `data.pri`）。一旦写成 `data/`：
>
> - 你新建的 `data/user_dao.cpp` 在 `git status` 里**根本不出现**，你以为提交了，其实没有；
> - 推上 GitHub 之后，另外三个人拉下来，工程里没有 DAO 实现，一编译全是 `LNK2019`，而且**他们本地怎么改都修不好**。
>
> 所以只忽略 exe 旁边生成的那一份，用**从仓库根开始的完整路径**（前面带 `/`，不带就是通配）：`/Second-handTradingPlatform/data/app.db`、`/Second-handTradingPlatform/data/images/`。
>
> **提交前用这条命令自检**：`git status --short --untracked-files=all`，`Second-handTradingPlatform/data/` 下的 `*.h`、`*.cpp`、`*.pri` 必须都在列表里。看不到就是被忽略了。

**顺手再建一个 `.editorconfig`**（同样放在 `Second-handTradingPlatformDev/` 下）：

```ini
root = true

[*]
charset = utf-8
insert_final_newline = true
trim_trailing_whitespace = true
indent_style = space
indent_size = 4

[*.md]
trim_trailing_whitespace = false

[*.{sln,vcxproj,filters,pro,pri}]
indent_size = 2
```

VS 2022 自带支持，不用装插件。它管的是**新建文件时的编码和缩进**：有了 `charset = utf-8`，在 VS 里新建的 `.cpp` 直接就是 UTF-8，不会再出现下面这种连环报错：

```
C4828 文件包含在偏移 0x101 处开始的字符，该字符在当前源字符集中无效(代码页 65001)
C2001 常量中有换行符
```

原因很简单：VS 在中文 Windows 上新建文件时，默认按**系统编码 GBK** 存；而 `.pro` 里开了 `/utf-8`，编译器按 UTF-8 去读 GBK 的字节，中文字符串就被读坏了。**没有这个文件时，你每新建一个带中文的 `.cpp` 都要手动存成 UTF-8。**

然后初始化并提交：

```bash
git init
git status
git add .
git status
git commit -m "chore:初始化仓库（文档 + 空 Qt 工程）"
```

**这几条命令分别在做什么**

| 命令 | 作用 |
| --- | --- |
| `git init` | 在当前目录建一个 `.git` 隐藏目录，从这里开始 Git 才管这个文件夹 |
| `git status` | 看当前状态：哪些文件改了、哪些已经准备好提交 |
| `git add .` | 把当前目录下所有改动放进“暂存区”，准备提交 |
| `git commit -m "..."` | 把暂存区的东西打包成一个提交，存进 `.git` 的历史里 |

`add` 之后再看一次 `git status` 很重要。这一步能看到到底有哪些文件要被提交，如果里面出现 `build/`、`debug/`、`*.exe`、`ui_mainwindow.h`，说明 `.gitignore` 没生效，先别 commit。

> ⚠️ `.gitignore` 没生效最常见的原因是文件名存成了 `.gitignore.txt`，Windows 资源管理器默认隐藏扩展名。在 Git Bash 里 `ls -a` 看全名。已经误提交的话，执行 `git rm -r --cached build debug` 把它从 Git 的索引里去掉，再 `git add .` 重新提交。

### 1.4 关联 GitHub 仓库并推送

```bash
git remote add origin https://github.com/你的账号/Second-handTradingPlatform.git
git remote -v
git branch -M main
git push -u origin main
```

**逐条解释**

| 命令 | 作用 |
| --- | --- |
| `git remote add origin <地址>` | 给远程仓库起个名字叫 `origin`，以后 `push`、`pull` 都写这个名字，不用每次贴长地址 |
| `git remote -v` | 检查有没有加上，正常会打印一行 fetch 一行 push |
| `git branch -M main` | 把当前分支改名成 `main`，`-M` 是强制改名。Git 老版本默认叫 `master`，这里统一成 `main` |
| `git push -u origin main` | 把 `main` 分支推到 `origin`。`-u` 是记住对应关系，以后在这个分支上直接 `git push` 就行 |

第一次推送会弹出浏览器要你登录 GitHub，点 `Authorize`。没弹窗就按终端提示输入账号和 Personal Access Token（密码方式 GitHub 早就不支持了）。

✅ **验证**：刷新网页，能看到 `README.md`、`docs/`、`Second-handTradingPlatform/`，**看不到** `build/`。

**如果 push 报这个**

```
fatal: unable to access 'https://github.com/...': Recv failure: Connection was reset
```

或者卡很久没反应、最后 `Connection timed out`：这不是账号问题，是**网络到 GitHub 被中断了**，国内网络很常见。很多人电脑上其实开着代理软件（Clash、v2rayN 之类），但 **git 不会自动走系统代理**，得单独告诉它。

先确认代理软件的端口（Clash Verge 默认 `7897`，Clash for Windows 默认 `7890`，v2rayN 默认 `10809`，软件设置界面里能看到），然后：

```bash
git config --global http.proxy  http://127.0.0.1:7897
git config --global https.proxy http://127.0.0.1:7897
git push -u origin main
```

代理软件**关掉以后**必须撤销这两条，否则 git 会去连一个不存在的端口，报 `Failed to connect to 127.0.0.1 port 7897`：

```bash
git config --global --unset http.proxy
git config --global --unset https.proxy
```

查当前有没有配：`git config --global --get http.proxy`，没输出就是没配。

### 1.5 建 develop 分支并保护它

```bash
git checkout -b develop
git push -u origin develop
git checkout main
```

**为什么要有 `develop`**

`main` 只放“随时能跑、能拿去答辩”的版本。四个人的代码先合到 `develop`，联调没问题了再一次性合进 `main`。这样即使某天 `develop` 被改坏了，`main` 还是好的，演示不会开天窗。

`git checkout -b develop` 的意思是“基于当前分支新建一条叫 `develop` 的分支并切过去”。刚建出来时它和 `main` 内容完全一样。

然后到 GitHub 网页上：

`Settings → Branches → Add branch protection rule`

对 `main` 和 `develop` **各做一次**，`Branch name pattern` 分别填 `main` 和 `develop`，勾选：

- [x] `Require a pull request before merging`
  - [x] `Require approvals` → 填 `1`
  - [x] `Dismiss stale pull request approvals when new commits are pushed`
  - [x] `Require approval of the most recent reviewable push`
- [x] `Do not allow bypassing the above settings`

**每个勾选项在管什么**

| 选项 | 作用 |
| --- | --- |
| `Require a pull request before merging` | 不许直接往这条分支推，只能走 PR |
| `Require approvals` = 1 | 至少一个人点过 Approve 才能合并 |
| `Dismiss stale pull request approvals...` | 批过之后作者又推了新代码，旧批准作废，得重新看 |
| `Require approval of the most recent reviewable push` | 最后一个推代码的人不能自己批自己，防止一个人自开自批自合 |
| `Do not allow bypassing the above settings` | 管理员也不能绕过 |

不要勾 `Require review from Code Owners`：你们没有 `CODEOWNERS` 文件，勾了以后 PR 会一直卡在“等待负责人批准”，永远合不进去。

**然后立刻切到你自己的工作分支**（Day 2~4 的所有提交都在这条分支上，因为 `main` 和 `develop` 已经被保护，直接 push 会被拒）：

```bash
git checkout -b feature/D-base
git push -u origin feature/D-base
```

分支名 `feature/D-base` 拆开看：`feature/` 表示这是一条功能开发分支，`D` 是负责人，`base` 是这块活的名字（平台底座）。另外三条按同样的规则起：`feature/A-product`、`feature/B-order`、`feature/C-trade`。

> Day 1 到此结束。`main` 上只有第一个提交，`develop` 是同一个起点，Day 2~4 的提交都进 `feature/D-base`，第五天再一次性 PR 合进 `develop`。

---

## Day 2 · 目录骨架、`.pro` 和四个 `.pri`

### 2.1 建目录

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

`mkdir -p` 里的 `-p` 是 parents，表示一层层建上去，父目录不存在也顺手建了，已经存在也不报错。

**每个目录装什么**

| 目录 | 装什么 | 谁负责 |
| --- | --- | --- |
| `core/common/` | 全项目共用的东西：`OpResult`、状态常量 | D |
| `core/entity/` | 数据模型结构体：商品、用户、订单、日志 | A/B/C 各一个 |
| `core/repository/` | 四个仓库接口，只有声明 | A/B/C 各一个 |
| `data/` | DAO 实现和数据库管理器 | D 一个，A/B/C 各一个 |
| `services/` | 业务逻辑 | 每人自己的 |
| `ui/pages/` | 页面，按业务域分子目录 | 各人自己的 |
| `ui/resources/` | `.qrc` 资源清单 | D |
| `sql/` | 建库脚本 | D |
| `assets/demo/` | 演示用的 CSV 样例 | D |

Git 不跟踪空目录，所以现在建完先不提交，等 2.4 放完占位文件一起提交。

### 2.2 写 `Second-handTradingPlatform.pro`

整个文件替换：

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

**每一行在说什么**

| 行 | 作用 |
| --- | --- |
| `QT += core gui widgets sql` | 声明要用 Qt 的哪几个模块。`sql` 是数据库，不加这一行 `#include <QSqlDatabase>` 会直接报找不到头文件 |
| `CONFIG += c++17` | 用 C++17。项目里用了 `inline` 变量、`if constexpr` 之类的新语法 |
| `TEMPLATE = app` | 生成一个可执行程序（不是库） |
| `TARGET = Second-handTradingPlatform` | 生成的 exe 名字。不加这行会用目录名，容易和目标名对不上 |
| `INCLUDEPATH += $$PWD` | 把工程根目录加进头文件搜索路径，这样任何文件里都能写 `#include "core/entity/product.h"`，不用写 `../../core/...` |
| `msvc { QMAKE_CXXFLAGS += /utf-8 }` | 只在 MSVC 编译器下加 `/utf-8`，让源码里的中文按 UTF-8 解释，否则界面上是乱码 |
| `SOURCES += main.cpp` | 先只加 `main.cpp`，其余的靠下面四个 `.pri` 各自声明 |
| `include(...)` | 把四个 `.pri` 的内容原样拼进来 |
| `RESOURCES += ui/resources/resources.qrc` | 资源文件。只在这一处写一次，`.pri` 里不要再 glob `*.qrc`，否则资源被注册两遍 |
| `DESTDIR` 那一段 | 按 Debug / Release 把 exe 放到 `debug/` 或 `release/`，两个 IDE 编出来的东西就不会各散一处 |

`CONFIG(debug, debug|release)` 是 qmake 的条件写法，意思是“如果当前配置是 debug（在 debug 和 release 之间挑），就走上面那块，否则走 else”。

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

**在说什么**

`HEADERS` 是头文件清单，`SOURCES` 是源文件清单，`FORMS` 是 `.ui` 界面文件清单。VS 和 moc 都要靠这三个清单才知道该处理哪些文件。

`$$files($$PWD/*.h, true)` 的第二个参数 `true` 是递归，所以 `ui/ui.pri` 里一行就能把 `ui/pages/product/search_page.h` 这种深层文件也收进来，不用给每个子目录写一行。

这样分工有好处：谁新增了文件都不用改 `.pro`，只要重新跑一次 qmake（VS 里的操作是 `扩展 → Qt VS Tools → Open Qt Project File (.pro)`）。反过来说，**新建文件后忘了重新生成工程，会报 `LNK2019 无法解析的外部符号`**，因为新文件根本没被收进去。

### 2.4 放占位文件

给每个人建一个只有 `#pragma once` 的空头文件，让目录非空、让工程能编过：

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

`echo "文本" > 文件名` 是“把这段文本写进文件”，目录里原本没有这个文件就新建。

`#pragma once` 是头文件保护：同一个头文件被 `#include` 多次时只处理一遍。等价于传统的 `#ifndef` + `#define` + `#endif` 三件套，但不会写错名字。

> ⚠️ 这些占位文件不要加 `#include`，也不要加类定义。它们的作用只是占住目录、让 Day 3 能编译通过。各自负责的人会在自己分支上把它们写成真正的实现，D 不要去改它们的内容。

### 2.5 把模板生成的 `mainwindow.*` 挪进 `ui/`

Qt 创建工程时会自动在**工程根目录**生成三个文件：`mainwindow.h`、`mainwindow.cpp`、`mainwindow.ui`。我们的约定是主窗口放在 `ui/` 下，所以先挪过去：

```bash
cd /d/DSH-workspace/Second-handTradingPlatformDev/Second-handTradingPlatform

git mv mainwindow.h  ui/mainwindow.h
git mv mainwindow.cpp ui/mainwindow.cpp
git mv mainwindow.ui  ui/mainwindow.ui

# 根目录那个 ui_mainwindow.h 是生成物，删掉，重新生成时会出现在该在的位置
rm -f ui_mainwindow.h
```

然后把 `main.cpp` 的 include 改掉（**只改这一行**）：

```cpp
#include "ui/mainwindow.h"          // 原来是 "mainwindow.h"
```

| 命令/改动 | 作用 |
| --- | --- |
| `git mv 旧 新` | 移动文件并告诉 Git 这是“改名”而不是“删一个加一个”，历史记录不断 |
| `rm -f ui_mainwindow.h` | 删掉旧的生成头文件。它是 uic 生成物，应该被 `.gitignore` 忽略，留着容易和新生成的搞混 |
| `#include "ui/mainwindow.h"` | `INCLUDEPATH += $$PWD` 让工程根目录进入搜索路径，所以从根目录出发写完整相对路径就能找到 |

**为什么必须挪**

`.pro` 里 `SOURCES` 只显式列了 `main.cpp`，其余文件全靠 `core.pri`、`data.pri`、`services.pri`、`ui.pri` 四个文件用 `$$files()` 通配。而 `ui.pri` 通配的是 `ui/` 目录（`$$PWD` 在 `ui.pri` 里就是 `ui/`）。

根目录的 `mainwindow.cpp` 不在任何一个通配范围里，于是 qmake 生成的工程文件里根本没有它，编译时也就不编译它。

**你刚才看到的那三个错误就是这么来的**：

```
LNK2019 无法解析的外部符号 MainWindow::MainWindow(class QWidget *)
LNK2019 无法解析的外部符号 MainWindow::~MainWindow(void)
LNK1120 2 个无法解析的外部命令
```

`main.cpp` 里写了 `MainWindow w;`，编译器需要构造函数和析构函数的**实现**。声明在 `ui/mainwindow.h` 里（能找到），实现应该在 `mainwindow.cpp` 里，但那个文件没被编进工程，链接器找不到实现，就报“无法解析的外部符号”。

`LNK2019` 的通用含义就是：**有人用了这个东西，但没人提供它**。排查顺序永远是：

1. 实现文件真的写了吗（打开看一眼，不是空的）
2. 实现文件进工程了吗（`.vcxproj` 里有没有它）
3. 进了工程但没重新生成（`.pri` 的通配符要 qmake 重跑才生效）

这次是第 2 条。

### 2.6 验证空工程能编译

1. VS 里 `文件 → 打开 → 项目/解决方案`，选 `Second-handTradingPlatform/Second-handTradingPlatform.pro`
2. 提示转换时点 **是**，等它生成 `.sln` 和 `.vcxproj`
3. 配置选 `Debug` + `x64`
4. `Ctrl+Shift+B`，应该 0 错误

此时界面还是 Qt 模板的样子（一个空白主窗口），能编过就算成功。

> 如果 VS 里工程已经开着，挪完文件后**必须重新走一次转换**：`扩展 → Qt VS Tools → Open Qt Project File (.pro)`。
> 不重新转换，`.vcxproj` 里还是旧的清单，照样报 `LNK2019`。这一步以后每次挪动、新增文件都要做，习惯就好。

**为什么 VS 要“转换”**

`.pro` 是 qmake 的格式，VS 不认。Qt VS Tools 读 `.pro`，把它翻译成 `.vcxproj`，之后 VS 就用自己的一套来编译。所以每次改了 `.pro`、`.pri`、`.ui`，或者**新增、移动了源文件**，都要重新走一次这个转换，相当于让 qmake 重新算一遍文件清单。

这一步失败，99% 是 `.pro` 里 `include()` 的路径写错，或者 `.pri` 文件名拼错。qmake 的报错会直接指出是哪一行。

### 2.7 提交 Day 2

```bash
git add .
git commit -m "feat(build):建立目录骨架、.pro 与四个 .pri"
git push
```

提交信息里 `feat(build)` 的括号表示“这次改动的范围是构建配置”，只是给人看的补充，不是 Git 的语法要求。你们约定只用 `feat:` / `fix:` / `docs:` / `test:` 四种前缀，括号可有可无。

---

## Day 3 · 公共头文件、建库 SQL、资源文件

这一天的产出是三个“定死”的东西：文件名的返回值类型 `OpResult`、全项目的状态常量、建库脚本。它们一旦定了，四个人的代码都按它写，谁改谁请客。

### 3.1 `core/common/opresult.h`

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

**逐段讲解**

| 片段 | 作用 |
| --- | --- |
| `struct OpResult` | 定义一个新的数据类型，里面装三个东西。用 `struct` 而不是 `class`，因为成员全是公开的，没有隐藏状态 |
| `bool ok = false;` | 成功还是失败。`= false` 是默认值，`OpResult r;` 造出来默认就是失败，忘了赋值也不会误报成功 |
| `QString message;` | 失败原因，比如“价格必须大于 0”。界面直接拿它弹提示框，不用再翻译 |
| `QString value;` | 成功时顺带带回一个值，比如新插入的商品 id，或者生成的订单号 |
| `static OpResult success(...)` | `static` 表示这是“属于类型本身”的函数，不用先有对象就能调用：`OpResult::success("12")` |
| `const QString& v = QString()` | 参数是常量引用，不拷贝字符串；`= QString()` 是默认参数，不传就是空串 |
| `fail(const QString& m)` | 出错时用，`m` 是 message 的缩写 |

**为什么不抛异常**

Qt 的代码风格里很少用异常，大部分 Qt 函数用返回值表示成功失败（比如 `open()` 返回 bool）。四个人都是新手，异常容易漏 catch 导致程序直接崩，而且 `QSqlDatabase` 在异常展开时的清理顺序不直观。统一用一个结构体返回，每个调用点都必须看一眼 `ok`，反而更难漏错。

`OpResult` 这个名字读作 operation result，操作结果。它只在 `core/common/` 里定义一次，谁都能 include，谁都不许改。

### 3.2 `core/common/constants.h`

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

**这段话在解决什么问题**

如果每个人在自己代码里直接写 `"SOLD"`、`"PAID"` 这种字符串，迟早有人写成 `"Sold"`、`"sold"`，编译不报错，运行时数据对不上，查一整天。所以全项目只在这一个文件里出现这些字面量，别处一律写 `Const::OrderStatus::Paid`。写错了编译器立刻报错。

| 片段 | 作用 |
| --- | --- |
| `namespace Const {` | 命名空间，把这一堆常量圈在一起，避免和别人的名字撞车 |
| 里面的 `namespace ProductStatus { ... }` | 再分一层，商品状态和订单状态都有 `Sold`/`Paid` 这种词，分层之后可以重名 |
| `inline const QString OnSale = "ON_SALE";` | 定义一个常量字符串。`inline` 是 C++17 的写法，允许把变量定义写在头文件里被多处包含而不报重复定义 |
| `const` | 只读，谁都不能给它赋新值 |
| `inline QString zh(const QString& code)` | 把状态码翻译成中文的函数。`zh` 就是中文的拼音缩写 |
| `static const QHash<QString, QString> m = {...}` | 函数内的静态变量，整个程序只构造一次，之后每次调用复用。`QHash` 是哈希表，`key` 是状态码，`value` 是中文 |
| `m.value(code, code)` | 查表，找到了返回中文，没找到就原样返回传入的 `code`，不会返回空 |

**为什么用字符串不用 enum**

数据库里 `status` 列直接存 `'PAID'`，打开 DB 文件一眼能看懂，答辩时把数据库打开指着讲比分不清的数字直观得多。enum 在数据库里会变成 0、1、2，还得记对应关系。

代价是编译器不会帮你检查拼写，所以约束变成了“只能从 `Const::` 里取”，靠的是 code review，不是编译器。这也是你们要互相 review 的一个实实在在的理由。

### 3.3 `sql/001_init.sql`

完整文件，一个字别改：

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

**每条语句在干什么**

| 语句 | 作用 |
| --- | --- |
| `PRAGMA foreign_keys = ON;` | 打开外键约束。SQLite 默认不检查外键，不打开的话 `seller_id` 填一个不存在的用户也能插进去 |
| `CREATE TABLE users (...)` | 建用户表 |
| `INTEGER PRIMARY KEY AUTOINCREMENT` | 自增主键。插入时不写 id，SQLite 自动给 1、2、3…，删掉的行号不会复用 |
| `TEXT NOT NULL UNIQUE` | 必须是文本，不能空，且不能重复。用户名唯一就靠它 |
| `DEFAULT (datetime('now','localtime'))` | 插入时不写创建时间就自动填当前时间。`localtime` 一定要加，不加的话存的是 UTC，比北京时间晚 8 小时 |
| `REAL` | 小数类型，价格用它。注意浮点数不适合做金融计算，课程设计这样够用 |
| `CHECK (price >= 0)` | 数据库层面的校验，插负数会直接失败 |
| `REFERENCES users(id)` | 外键，指向用户表。配合第 5 行那条 `PRAGMA` 才真正生效 |
| `categories.parent_id REFERENCES categories(id)` | 自引用外键。父分类的 `parent_id` 是空的，子分类指向父分类，就是一棵两层的树 |
| `CREATE INDEX idx_xxx` | 建索引，加快按这一列查询的速度。订单多了以后按买家查会快很多 |
| `INSERT INTO categories ...` | 插入 8 条初始分类，id 写死成 1~8 |

**表之间的关系**

```
users ──┬── products (seller_id)
        ├── orders   (buyer_id / seller_id)
        └── trade_log(operator_id)

categories ── products (category_id)
orders ── trade_log (order_id)
products ── orders (product_id)
```

订单里同时存 `buyer_id` 和 `seller_id`，是故意的冗余。这样查“我卖出的”不用绕回商品表，而且商品被删掉以后历史订单还知道当时是谁卖给谁的。

**⚠️ 三条硬规矩**（`runSqlScript` 的实现依赖它们）

1. **不要写行内注释**。`-- 注释` 只能单独占一整行，因为脚本是按 `;` 切的，行内注释里的分号会把语句切断
2. 每条语句以 `;` 结尾，最后一条也要
3. 分类 id 写死 1~8，四个人的代码和文档都按这 8 个数字写

### 3.4 `ui/resources/resources.qrc`

```xml
<RCC>
    <qresource prefix="/">
        <file alias="sql/001_init.sql">../../sql/001_init.sql</file>
    </qresource>
</RCC>
```

`.qrc` 是资源清单，告诉 rcc “把哪个磁盘文件编进 exe 里，编进去之后叫什么名字”。

| 属性 | 含义 |
| --- | --- |
| `prefix="/"` | 资源在程序里的根路径 |
| `alias="sql/001_init.sql"` | 编进去之后用 `:/sql/001_init.sql` 访问它 |
| `>../../sql/001_init.sql<` | 磁盘上的真实位置，相对 `.qrc` 文件所在目录（`ui/resources/`），所以要退两级回到工程根 |

把 SQL 编进 exe 的好处：程序不再依赖外面的文件，拷到别人电脑上也能自动建库。代价是改了 SQL 必须重新编译，运行时的修改不算数。

改了这个文件后必须重新生成工程，否则资源内容不会更新。

### 3.5 验证资源真的进程序了

在 `main.cpp` 里临时加两行。**位置很关键**：必须放在 `QApplication a(argc, argv);` 之后、`return QApplication::exec();` 之前。

```cpp
#include <QFile>
#include <QDebug>
...
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qDebug() << "sql exists:" << QFile::exists(":/sql/001_init.sql");   // ← 加在这里

    MainWindow w;
    w.show();
    return QApplication::exec();      // ← 这一行之后的代码永远不会执行
}
```

> **坑**：`return QApplication::exec();` 是程序的终点。写在它**下面**的代码永远不会跑，因为 `exec()` 会一直阻塞在那里跑事件循环，直到最后一个窗口关闭才返回，返回后 `main` 立刻结束。
> 你如果把这行贴到 `return` 后面，程序表现就是“一切正常，但什么都没打印”。

F5 运行，然后看 **`输出` 窗口**：上方有个下拉框，默认显示 `生成`，要切到 **`调试`**。里面应该出现：

```
sql exists: true
```

`qDebug()` 的输出走的是调试器的输出通道，所以：

- 必须按 **F5**（进入调试）。`Ctrl+F5` 只是“启动不调试”，收不到 `qDebug`
- 看的是 `输出 → 调试` 面板，不是 `输出 → 生成`，也不是终端

如果是 `false`，按这个顺序查：`.qrc` 有没有写进 `.pro` 的 `RESOURCES`、`../../sql/` 这个相对路径对不对、有没有重新生成工程。

**更稳的替代办法**（不想折腾调试输出时用这个）：

```cpp
#include <QMessageBox>
...
QMessageBox::information(nullptr, "检查",
    QFile::exists(":/sql/001_init.sql") ? "sql exists: true" : "sql exists: false");
```

弹窗写着 `true` 就通过，验证完把这行删掉。

### 3.6 提交 Day 3

```bash
git add .
git commit -m "feat(core):添加 OpResult、常量定义、建库 SQL 与资源文件"
git push
```

---

## Day 4 · `DbManager`：真正把库建出来

这一天的东西是整个项目的入口。后面的 DAO 全都通过 `DbManager::db()` 拿数据库连接，拿不到连接，谁的代码都跑不起来。

### 4.1 `data/db_manager.h`

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

**为什么整个类都是 static**

`DbManager` 不需要有实例。它管的是一条全局唯一的数据库连接，造两个对象出来反而让人以为有两个库。全部写成 `static` 成员函数，调用方式就是 `DbManager::init(&err)`，看代码的人一眼知道这是“工具类”。

| 声明 | 含义 |
| --- | --- |
| `static bool init(QString* errMsg = nullptr)` | 初始化。`QString*` 是“把失败原因写到你给我的这个变量里”，这是 C 风格的输出参数，Qt 里很常见。`= nullptr` 表示可以不传 |
| `static QSqlDatabase& db()` | 返回数据库连接。返回**引用**而不是值，因为连接对象不能被复制，复制出来的副本操作的不是同一条连接 |
| `static QString dbPath()` | 数据库文件的绝对路径，状态栏显示和报错时用 |
| `static void dumpSchema()` | 调试用，打印表名和分类，只在 Debug 版有效 |

头文件里只写声明，不写实现，实现放在 `.cpp`。这样改实现不会牵连所有 include 它的文件，编译更快。

### 4.2 `data/db_manager.cpp`

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
                     << "parent=" << q.value(2).toString();
        }
    }
#endif
}
```

**先看结构，再看细节**

```
namespace { ... }        匿名命名空间，里面的 runSqlScript 只在本文件可见
DbManager::dbPath()      算出 db 文件路径
DbManager::db()          拿到那条唯一连接
DbManager::init()        建目录 → 打开库 → 设 PRAGMA → 没有表就建表
DbManager::dumpSchema()  Debug 版才编译的调试打印
```

| 片段 | 作用 |
| --- | --- |
| `namespace { ... }` | 匿名命名空间。里面的函数只在当前 `.cpp` 内可见，别的文件调不到，和 `static` 函数一个意思。放这种“只给本文件用的小工具” |
| `bool runSqlScript(...)` | 读资源里的 SQL 文本，按分号拆成一条条，逐条执行 |
| `QFile f(resourcePath);` | 打开文件。`:/sql/001_init.sql` 这种带冒号的路径走 Qt 的资源系统，不是磁盘路径 |
| `f.open(QIODevice::ReadOnly)` | 以只读方式打开，返回 false 表示失败，必须检查 |
| `if (errMsg) *errMsg = ...` | 先判断指针非空再解引用。调用者可能不关心错误原因，传了 `nullptr` 也不该崩 |
| `QString::fromUtf8(f.readAll())` | 把文件的字节按 UTF-8 解释成字符串。SQL 文件里有中文分类名，编码错了会变乱码 |
| `text.split('\n')` | 按行切开 |
| `raw.trimmed()` | 去掉首尾空白，用来判断这一行是不是注释 |
| `line.startsWith("--")` | 是注释就 `continue` 跳过，不放进 cleanLines |
| `cleanLines << raw;` | `<<` 是往列表尾部追加 |
| `cleanLines.join('\n')` | 再把行拼回一整段文本 |
| `text.split(';', Qt::SkipEmptyParts)` | 按分号切成一条条 SQL，空的直接丢掉 |
| `QSqlQuery q(db);` | 造一个查询对象，绑定到某条连接上 |
| `q.exec(stmt)` | 执行这条 SQL，返回 false 表示失败 |
| `q.lastError().text()` | 拿到数据库给的具体错误信息，比如“near ";": syntax error” |
| `stmt.left(200)` | 取语句前 200 个字符放进错误信息，太长刷屏 |
| `QStringLiteral("...%1...").arg(x)` | `%1` 是占位符，`arg()` 按顺序填进去。多个参数时用逗号隔着一次传完 |
| `QCoreApplication::applicationDirPath()` | exe 所在目录的绝对路径 |
| `QDir().mkpath(dir)` | 逐级创建目录，已存在返回 true |
| `QSqlDatabase::addDatabase("QSQLITE", "main")` | 注册一个 SQLite 连接，名字叫 `main`。第二个参数是连接名，不传会用默认名 |
| `conn.setDatabaseName(dbPath())` | 指定数据库文件的位置。文件不存在时，第一次 `open()` 会自动创建 |
| `conn.open()` | 真正打开。这一步失败的原因通常是 SQLite 驱动没找到 |
| `static QSqlDatabase conn = QSqlDatabase::database("main");` | 函数内静态变量，第一次调用时查一次连接，之后直接返回。所以 `db()` 反复调用也是同一条连接 |
| `q.exec("PRAGMA foreign_keys = ON")` | 每条连接都要设一次，SQLite 默认不检查外键 |
| `q.exec("PRAGMA journal_mode = WAL")` | 换成 WAL 日志模式，读写不互相卡，程序崩了也不容易坏库 |
| `sqlite_master` | SQLite 自带的系统表，记录有哪些表。查 `users` 在不在，就知道库建过没有 |
| `q.next()` | 把结果游标移到下一行，返回 false 表示没有数据了 |
| `q.value(0).toInt()` | 取当前行第 0 列，转成整数 |
| `#ifdef QT_DEBUG ... #endif` | 条件编译，只有 Debug 配置才编译中间这段。Release 版里 `dumpSchema()` 是空函数 |

**为什么用 `applicationDirPath()` 而不是相对路径**

在 VS 里按 F5 调试时，程序的“当前工作目录”是工程目录，exe 却在 `debug\` 下面。写相对路径 `data/app.db`，调试时库建在工程目录，直接双击 exe 时又建在别处，于是出现“数据库里明明有数据，程序里查不到”。

`applicationDirPath()` 返回 exe 真正的所在目录，两种情况都是同一个位置。这个坑几乎每个人都会踩一次。

### 4.3 `main.cpp`

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

| 片段 | 作用 |
| --- | --- |
| `int main(int argc, char* argv[])` | 程序入口，`argc`/`argv` 是命令行参数个数和内容，Qt 要拿它处理一些启动选项 |
| `QApplication app(argc, argv);` | 每个 Qt 界面程序必须有且只有一个。它管事件循环、字体、样式等全局的东西 |
| `setApplicationName(...)` | 设置程序名，任务管理器和某些系统提示里会显示 |
| `QString err; DbManager::init(&err)` | 传地址进去，失败了 `err` 里就有原因 |
| `QMessageBox::critical(nullptr, 标题, 内容)` | 弹一个红色错误框。第一个参数是父窗口，还没有窗口就传 `nullptr` |
| `return 1;` | 出错退出。返回 0 表示正常，非 0 表示异常，脚本和快捷方式能据此判断 |
| `MainWindow w;` | 在栈上创建主窗口。`main` 返回时它自动析构，不需要 `delete` |
| `w.show();` | 显示窗口。只创建不 show 是看不见的 |
| `return app.exec();` | 进入事件循环，程序停在这里等鼠标键盘事件。窗口关掉、`exec()` 返回，程序才结束 |

**为什么先初始化数据库再建窗口**

数据库打不开的话，后面每个页面查数据都会失败，用户看到一堆报错。先在入口处把这一关过了，失败就弹框退出，界面里就不用到处判断“数据库准备好了没有”。

### 4.4 第一次跑起来

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
```

5. 去目录里看，`Second-handTradingPlatform/debug/data/app.db` 真的存在
6. 关掉程序再运行一次，输出变成 `使用已有数据库`，说明不会重复建表

`sqlite_sequence` 是 SQLite 自己维护的自增计数表，出现在表列表里正常。

如果第 3 步连窗口都没弹出来、输出面板里一行 `[DbManager]` 都没有，先别改代码 —— 直接看下一节 4.5，八成是缺 `Qt6Sqld.dll`。

### 4.5 先补一次 `windeployqt`（Debug 版也要）

到这一步工程第一次用到 `QSqlDatabase`，`debug\` 目录里就多出一个必需要件：**`Qt6Sqld.dll`**，以及插件目录 **`sqldrivers\`**。少了它们，现象很隐蔽：

- 编译 0 错误，`Ctrl+Shift+B` 每次都成功；
- 但 `F5` 之后**什么都没发生**，或者窗口一闪就没了；
- 输出面板里连一行 `[DbManager]` 都没有 —— 因为程序在加载 Qt 库那一步就没起来，`main()` 根本没执行。

不是代码错了，是**缺 dll**。补一次就好（`D:\Qt6\6.11.2\msvc2022_64` 换成你自己的 Qt 路径）：

```
"D:\Qt6\6.11.2\msvc2022_64\bin\windeployqt.exe" --debug --no-translations ^
    "D:\...\Second-handTradingPlatform\debug\Second-handTradingPlatform.exe"
```

| 参数 | 含义 |
| --- | --- |
| `--debug` | 按 Debug 版依赖来拷（Debug 版的库名带 `d` 后缀，如 `Qt6Sqld.dll`）。Release 版要用 `--release` |
| `--no-translations` | 不拷一堆语言翻译文件，省几十 MB |
| 最后那个路径 | 要分析的可执行文件，必须指向 `debug\` 里的那个 exe |

跑完 `debug\` 里会多出 `Qt6Sqld.dll` 和 `sqldrivers\qsqlited.dll`。之后 `F5` 就正常了。

> 只想调试、不想拷 dll 也行：把 `D:\Qt6\6.11.2\msvc2022_64\bin` 加进系统环境变量 `PATH`，Qt 会直接去那里找库。但**双击 exe 给同学看的时候还是会缺**，所以打包那一步（第 3 周）无论如何都要跑一次 `windeployqt`。

### 4.6 报错对照表

| 输出/现象 | 原因 | 解决 |
| --- | --- | --- |
| 编译通过但双击/F5 没反应、无任何输出 | `debug\` 缺 `Qt6Sqld.dll` 和 `sqldrivers\`（第 4 步才用到 `QSqlDatabase`） | 跑一次本节 4.5 的 `windeployqt --debug ...` |
| `C4828` / `C2001 常量中有换行符` | 源文件不是 UTF-8（从记事本或网页粘过来常是 GBK），而 `.pro` 里开了 `/utf-8` | VS 里打开该文件 → `文件 → 高级保存选项` → 编码选 **UTF-8（无签名）** → 保存 |
| `C2039 "toVariant": 不是 "QVariant" 的成员` | 复制了 `q.value(2).toVariant()` 这类写法，`QVariant` 本来就没有 `toVariant()` | 直接用 `q.value(2).toString()` |
| `driver not loaded` | SQLite 插件没在 exe 旁边 | 把 `D:\Qt6\6.11.2\msvc2022_64\plugins\sqldrivers\` 整个目录拷到 `debug\` 下 |
| `无法打开内置 SQL 脚本` | 资源没编进去 | 检查 `.pro` 的 `RESOURCES`、`.qrc` 里的相对路径，重新生成工程 |
| `建表失败：... no such table` | 语句顺序或分号问题 | 看错误里附带的“出错语句”，对照那条 SQL 改 |
| `表：(...)` 里没有表 | 命中了“已有数据库”分支但库是空的 | 删掉 `debug/data/app.db` 重跑 |
| 编译报 `C1083: QSqlDatabase` | `.pro` 少 `QT += sql` | 加上后重新生成工程 |

### 4.7 提交 Day 4

```bash
git add .
git commit -m "feat(data):DbManager 完成自动建库与外键配置"
git push
```

---

## Day 5 · 主窗口骨架 + 通知全员

### 5.1 用 Designer 摆界面

打开 `ui/mainwindow.ui`（双击），做四件事：

1. 把默认的 central widget 删掉（`objectName` 是 `centralwidget` 的那个）
2. 从左边拖一个 `List Widget` 进来，**`objectName` 改成 `navList`**，`maximumWidth` 设 160
3. 再从 Containers 里拖一个 `Stacked Widget` 进来，**`objectName` 改成 `stackedWidget`**
4. 选中 centralwidget，点上方工具栏的 `水平布局`，让两个控件左右排开

**这两个控件是什么**

| 控件 | 作用 | 在这个程序里 |
| --- | --- | --- |
| `QListWidget` | 一列可点的条目 | 左边的导航菜单 |
| `QStackedWidget` | 一摞页面，同一时刻只显示一个 | 右边的内容区，五个页面叠在里面 |

`objectName` 是控件在代码里的名字。Designer 里设成 `navList`，代码里就能用 `ui->navList` 访问它。**必须一个字不差**，大小写也要一致，不要用中文当 `objectName`。

保存，回 VS 执行 `扩展 → Qt VS Tools → Open Qt Project File (.pro)` 重新生成一次。`.ui` 改了必须做这一步，否则 `ui_mainwindow.h` 还是旧的。

### 5.2 `ui/mainwindow.h`

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

| 片段 | 作用 |
| --- | --- |
| `namespace Ui { class MainWindow; }` | 前置声明。`ui_mainwindow.h` 里生成的类叫 `Ui::MainWindow`，和外面的 `::MainWindow` 不是一个东西，名字撞车所以包了一层命名空间 |
| `QT_BEGIN_NAMESPACE` / `QT_END_NAMESPACE` | Qt 的兼容宏。普通构建下它们展开成空，某些特殊构建下会展开成 `namespace QT_NAMESPACE {`。照抄即可 |
| `class MainWindow : public QMainWindow` | 继承 `QMainWindow`，得到菜单栏、状态栏、工具栏这些现成的东西 |
| `Q_OBJECT` | 告诉 moc 这个类要用信号槽和 `tr()`。写了它就必须重新生成工程让 moc 处理 |
| `explicit MainWindow(QWidget* parent = nullptr)` | 构造函数。`explicit` 禁止隐式转换，防止有人写 `MainWindow w = nullptr;` 这种奇怪代码 |
| `~MainWindow() override` | 析构函数。`override` 让编译器检查确实重写了父类的虚函数，写错名字会报错而不是悄悄失效 |
| `Ui::MainWindow* ui;` | 指向 Designer 生成的那套控件的指针，`ui->navList` 就是这么来的 |
| `makePlaceholder(const QString& text)` | 造一个写着“待实现”的空页面，第 2 周换成真页面 |

### 5.3 `ui/mainwindow.cpp`（第 0 周版本）

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

| 片段 | 作用 |
| --- | --- |
| `#include "ui_mainwindow.h"` | Designer 生成的类在这里面，必须 include 才能 `new Ui::MainWindow` |
| `: QMainWindow(parent)` | 初始化列表里先构造父类 |
| `, ui(new Ui::MainWindow)` | 再构造成员。成员初始化顺序按声明顺序，不按这里写的顺序 |
| `ui->setupUi(this)` | 把 `.ui` 里摆好的控件挂到这个窗口上，同时给 `ui->navList` 这类指针赋值 |
| `resize(1100, 700)` | 初始窗口大小（宽 × 高） |
| `statusBar()->showMessage(...)` | 窗口底部那行状态栏显示文字，这里拿来显示数据库路径，调试时很方便 |
| `delete ui;` | 手动删掉 `ui`。它是普通对象，不挂在 Qt 对象树上，不删会泄漏 |
| `new QWidget(this)` | 造页面，`this` 是父对象，窗口销毁时它自动跟着销毁 |
| `new QVBoxLayout(page)` | 竖排布局，自动把里面的控件按顺序从上往下排 |
| `label->setAlignment(Qt::AlignCenter)` | 文字居中 |
| `QStringLiteral("【%1】\n（待实现）").arg(text)` | 拼字符串，`%1` 被替换成页面名 |
| `addWidget(...)` / `addItem(...)` | 往栈里加一页 / 往列表里加一项 |
| `setCurrentRow(0)` / `setCurrentIndex(0)` | 默认选中第一项、显示第一页 |
| `connect(发送者, &类::信号, 接收者, &类::槽)` | 建立连接。这里是“导航列表选中行变了 → 右侧栈跟着切页” |

**信号槽这一行怎么读**

```cpp
connect(ui->navList, &QListWidget::currentRowChanged,
        ui->stackedWidget, &QStackedWidget::setCurrentIndex);
```

`currentRowChanged(int)` 是 `QListWidget` 发出的信号，选中行变化时发出，带一个 int 参数表示新行号。`setCurrentIndex(int)` 是 `QStackedWidget` 的槽，正好接收一个 int。

Qt 5 之后这种写法在编译期就检查类型，参数对不上直接编译失败，比老式的 `SIGNAL()/SLOT()` 宏安全得多。`&类::函数名` 这种取函数指针的写法看着吓人，记住“它就是函数地址”就够了。

### 5.4 验收：第 0 周完成标志

- [ ] 点左边五个导航，右边切换页面，每页显示“（待实现）”
- [ ] 状态栏显示数据库绝对路径
- [ ] `debug/data/app.db` 存在，里面有 5 张表 + 8 条分类
- [ ] 输出窗口打印表名和分类条数
- [ ] `git clone` 到一个全新目录，重新打开 `.pro` 转换后 F5 **也能跑**

最后一条最重要。它说明仓库是自洽的：不依赖你本机的任何临时文件，别人拿到就能编出来。

### 5.5 推送并通知

你现在应该在 `feature/D-base` 上（1.5 末尾切过去的）。先确认：

```bash
git branch --show-current     # 应该打印 feature/D-base
git status                    # 工作区干净再往下走
git push                      # 推自己的分支，没人拦
```

`main` 和 `develop` 已经开了保护，不能直接 push，要走 PR：

1. 打开仓库网页，会看到黄色的 `Compare & pull request` 按钮，点它
2. 确认顶部两个下拉框是 **`base: develop` ← `compare: feature/D-base`**，写反了 PR 就合到错的地方
3. 标题写 `feat:第 0 周骨架（目录/.pro/DbManager/主窗口）`，点 `Create pull request`
4. 右侧 `Reviewers` 点一个人（A/B/C 谁都行），微信喊他“看下我的 PR，点 Approve”
5. 他点完 `Approve`，你点 `Merge pull request` → `Confirm merge`
6. 切回本地同步：

```bash
git checkout develop
git pull
```

> ⚠️ 找不到人 approve 的时候不要去关分支保护。保护的意义就是逼你们互相看一眼代码，答辩时“我们的 develop 分支所有代码都经过 review”这句话是有 PR 截图作证的。

然后在群里发：

> 骨架已推 develop：目录、.pro、四个 .pri、constants.h、opresult.h、001_init.sql、DbManager、主窗口（五个占位页）。
> 你们可以 `git checkout -b feature/xxx` 开工了。
> 改文件前先看 `docs/01-契约.md` 的目录归属表，`ui/mainwindow.*` 归我，别动。

### 5.6 建另外三个人的分支

`Branches → New branch`，分别建 `feature/A-product`、`feature/B-order`、`feature/C-trade`，源分支都选 **`develop`**。

（`feature/D-base` 你 1.5 就建好了，不用再建。）

建完在群里把两条命令贴给他们：

```bash
git fetch
git checkout -b feature/A-product origin/develop     # B/C 换成自己的分支名
```

`git fetch` 只把远程的分支信息拉到本地，不动你的工作文件；`git checkout -b 新分支 origin/develop` 表示以远程 `develop` 为起点建本地分支。

> 也有一条更省事的写法，效果一样：`git checkout feature/A-product`。前提是先 `git fetch`，因为 Git 要先知道远端有这么一个分支，才会自动在本地建同名分支并关联它。
>
> 用 `-b ... origin/develop` 这种写法时要注意：这个本地分支的“上游”暂时指向 `origin/develop`，所以**第一次推送必须带完整参数** `git push -u origin feature/A-product`（`-u` 会把上游改到自己的远端分支上）。不加 `-u` 直接 `git push` 会报 `The upstream branch of your current branch does not match the name of your current branch`。

第 0 周到此结束。接下来进 [D-施工步骤-第2周.md](D-施工步骤-第2周.md)。
