# 二手交易平台（Qt / C++ / SQLite 课程设计）

四个人用一个 Qt Widgets 桌面程序做一个**单机**二手交易平台：商品能发布、能搜索、能买、订单能一步步往下走、每一步都有记录可查。

不联网、不连服务器、不引第三方库，数据库就是一个 `app.db` 文件。

---

## 四块功能与负责人

| 功能 | 负责人 | 主要页面 |
|---|---|---|
| ① 商品上架 / 信息修改 / 下架 / 分类检索 | **A** | 商品检索页、发布对话框、我的商品页 |
| ② 寄售登记（注册）/ 下单 / 订单记录 | **B** | 登录注册框、购买确认框、我的订单页、订单详情 |
| ③ 交易状态跟踪 / 记录查询 | **C** | 交易记录页、（订单详情里的日志表和动作按钮） |
| ④ 界面与平台底座 | **D** | 主窗口、数据库、组装、批量导入页 |

---

## 文档怎么读

| 文件 | 什么时候看 |
|---|---|
| [docs/00-分工选择.md](docs/00-分工选择.md) | **第一天先看这个**。四张角色卡，谁挑哪个 |
| [docs/01-契约.md](docs/01-契约.md) | **第二份，所有人通读一遍**。表结构、接口签名、状态机、目录归属都在这 |
| [docs/02-环境与协作手册.md](docs/02-环境与协作手册.md) | 装环境、Git 怎么用、报错怎么查 |
| [docs/tasks/A-商品域.md](docs/tasks/A-商品域.md) | A 的任务清单和步骤 |
| [docs/tasks/B-用户与订单域.md](docs/tasks/B-用户与订单域.md) | B 的任务清单和步骤 |
| [docs/tasks/C-交易状态域.md](docs/tasks/C-交易状态域.md) | C 的任务清单和步骤 |
| [docs/tasks/D-平台底座.md](docs/tasks/D-平台底座.md) | D 的任务清单和步骤 |
| [docs/tasks/D-详细施工步骤.md](docs/tasks/D-详细施工步骤.md) | **D 动手时照这个做**：逐日逐文件，含全部可复制代码 |
| [docs/meetings/README.md](docs/meetings/README.md) | 每周开会照着这个记 |

> 契约是**唯一的裁判**。两份文档说法不一样时，以契约为准，然后去改另一份。

---

## 第一天要做的四件事

```bash
# 1. 关掉自动换行转换（Windows 上必做，否则每行都冲突）
git config --global core.autocrlf false

# 2. 克隆
git clone <D 发到群里的仓库地址>
cd Second-handTradingPlatform

# 3. 开自己的分支（把下面换成自己的名字）
git checkout -b feature/A-product      # B: feature/B-order  C: feature/C-trade  D: feature/D-base

# 4. 用 VS2022 打开 .sln，选 Debug x64，F5
```

跑出一个空窗口 = 环境通了。**跑不出来先别写代码，去群里问 D。**

---

## 时间安排（四周）

| 周 | 要做完什么 | 周末检查 |
|---|---|---|
| 第 0 周 | D 交付骨架（仓库、建表 SQL、`DbManager`、空主窗口）；其他人写好**头文件和接口**并提 PR | 四个人都能 `git clone` 后 F5 出窗口 |
| 第 1 周 | 各自的 DAO 写完，能用 SQL 查出数据（还没有界面） | 每个人至少一个 DAO 函数跑通 |
| 第 2 周 | 各自的页面写完，业务逻辑（下单事务、改状态事务）跑通 | 单机能走完"发布→搜索→购买→付款→发货→收货" |
| 第 3 周 | 联调、修 bug、准备演示 | 演示流程连续走三遍不出错 |
| 第 4 周 | 打包、写报告、彩排 | 拷到别人电脑上能跑 |

**每周日 21:00 是合并窗口**：各自把分支合并到 `develop`，冲突当场解决，不留到第二天。

---

## 接口归属表（谁改什么，别越界）

| 文件 | 归谁 | 说明 |
|---|---|---|
| `core/entity/product.h`、`data/product_dao.*`、`services/product_service.*`、`ui/pages/product/*` | **A 独占** | |
| `core/entity/user.h|order.h`、`core/session.h`、`data/user_dao.*`、`data/order_dao.*`、`services/auth_service.*`、`services/order_service.*`、`ui/pages/auth/*`、`ui/pages/order/*` | **B 独占** | |
| `core/entity/trade_log.h`、`data/trade_log_dao.*`、`services/trade_state_machine.*`、`services/trade_service.*`、`ui/pages/trade/*` | **C 独占** | |
| `core/common/*`、`data/db_manager.*`、`sql/*`、`.pro/.pri`、**`ui/mainwindow.*`**、`ui/pages/import/*`、`services/import_service.*`、`main.cpp` | **D 独占** | `mainwindow` 只能 D 改 |
| `docs/*` | 谁写谁改 | 改契约要先在群里说 |

**跨模块的调用只能通过 `core/repository/` 里的接口**：B 要改商品状态，调的是 `IProductRepository::updateStatus`，不是直接写 SQL。

---

## 演示流程（答辩就照这个走，5 分钟够）

1. **D**：打开程序 → 登录 `alice / 123456` → 点一下左边五个导航（说明这是五个页面）→ 说一句"数据库是一个本地 `app.db`，五张表"
2. **A**：发布一件新商品（填标题、选分类、填价格）→ 回检索页搜关键字 → 搜到了 → 切到"我的商品"改一下价格 → 检索页价格变了 → 下架它 → 检索页搜不到了
3. **B**：登录 `bob / 123456` → 搜到 alice 的商品 → 双击 → 确认购买 → 订单出现在"我买到的"。
   **接着再点一次同一件商品 → 提示"该商品已售出或已下架"**（这是重点，说明不是靠界面拦的）
4. **C**：进订单详情 → 付款 → 换 alice 登录 → 发货 → 换 bob 登录 → 确认收货 → 交易记录页看到四条日志；
   再说一句"已发货的订单没有取消按钮，因为状态机里就没有这条边"
5. **C/D**：另开一单 → 取消 → 回检索页，那件商品回到在售状态，别人还能买
6. **D**：批量导入一个 CSV（现场展示 10 行成功）→ 再导一个有一行错的（10 行全失败并指出是第几行）

演示前**先用"工具 → 重置演示数据"把数据恢复干净**，别现场手忙脚乱。

---

## 可能被问的问题

| 问题 | 怎么答 |
|---|---|
| 你们四个人怎么分工的？ | 按**业务域**分：商品、订单、状态、底座。每人一个分支、自己的文件，通过 `core/` 的接口和信号通信，`mainwindow.cpp` 是唯一的组装点 |
| 怎么证明是四个人做的？ | GitHub 的 Insights → Contributors / Network 图；每个人的 PR；`git shortlog -sne` |
| 为什么不用 MySQL / 服务器？ | 单机课程设计，SQLite 零配置、一个文件就能交作业；数据层做了 Repository 抽象，换数据库只改 DAO |
| 怎么防止同一件商品被卖两次？ | `UPDATE products SET status='SOLD' WHERE id=? AND status='ON_SALE'`，检查影响行数为 1；**在数据库层面拦，不是界面拦** |
| 取消了订单商品怎么办？ | 在同一个事务里把商品状态从 `SOLD` 改回 `ON_SALE`，所以不会出现"订单没了但商品锁着" |
| 用户角色怎么存的？ | **不存角色字段**。买家还是卖家由 `order.buyer_id / seller_id` 和当前用户比对推导出来 |
| 状态为什么用字符串不用枚举？ | 打开数据库能直接看懂；`Const::OrderStatus::Paid` 就是 `'PAID'`，答辩好讲 |
| 密码怎么存的？ | `sha256(随机盐 + 密码)`，每个用户一个盐，库里看不到明文 |
| 界面怎么这么简陋？ | 课程设计重点是**业务逻辑和数据一致性**（事务、状态机、并发更新），界面用 Qt 默认样式，功能完整优先 |

---

## 协作规矩（贴在群公告里）

1. 不直接往 `main` / `develop` 推，一律开 `feature/xxx` 分支提 PR
2. PR 至少一个人 review 通过才能合，**D 的 PR 由另外三个人中任意一个 review**
3. 提交信息前缀只用：`feat:` 新功能、`fix:` 修 bug、`docs:` 文档、`test:` 测试
4. **不提交编译产物**：`build/`、`debug/`、`release/`、`ui_*.h`、`*.db` 都在 `.gitignore` 里
5. 每次开工前先 `git checkout develop && git pull`，再切回自己的分支 `git merge develop`
6. 不要四个人用一台电脑、一个 GitHub 账号——那样协作记录就没了
7. 契约要改，先在群里说，改完通知所有人 `git pull`

---

## 目录结构

```
Second-handTradingPlatform/
├── main.cpp
├── Second-handTradingPlatform.pro
├── core/                      # 谁都能 include，谁都不许依赖上层
│   ├── core.pri
│   ├── common/                # opresult.h  constants.h          [D]
│   ├── entity/                # product.h(A) user.h order.h(B) trade_log.h(C)
│   ├── repository/            # 四个接口，各归各人
│   └── session.h                                              [B]
├── data/                      # 只认数据库，不认界面
│   ├── data.pri
│   ├── db_manager.h/.cpp                                      [D]
│   ├── product_dao.h/.cpp                                     [A]
│   ├── user_dao.h/.cpp  order_dao.h/.cpp                      [B]
│   └── trade_log_dao.h/.cpp                                   [C]
├── services/                  # 业务规则，不碰界面
│   ├── services.pri
│   ├── product_service.h/.cpp                                 [A]
│   ├── auth_service.h/.cpp  order_service.h/.cpp              [B]
│   ├── trade_state_machine.h/.cpp  trade_service.h/.cpp        [C]
│   └── import_service.h/.cpp                                  [D]
├── ui/
│   ├── ui.pri
│   ├── mainwindow.h/.cpp/.ui                                  [D]
│   ├── pages/product/  pages/auth/  pages/order/  pages/trade/  pages/import/
│   └── resources/resources.qrc      # 只装 001_init.sql
├── sql/001_init.sql                                           [D]
└── docs/
```

---

## 出问题了看这里

| 现象 | 原因 / 办法 |
|---|---|
| `QSQLITE driver not loaded` | 把 `D:\Qt6\6.11.2\msvc2022_64\plugins\sqldrivers\` 拷到 exe 旁边 |
| `C1083: 无法打开包括文件` | `.pri` 的通配符没生效，重跑 qmake；或用了 VS 就要重新生成工程 |
| `LNK2019 无法解析的外部符号` | 声明了没实现，或新加的 `.cpp` 没被收进工程 |
| `vtable for XXX` 未定义 | 类继承了 `QObject` 但没写 `Q_OBJECT`，或加了 `Q_OBJECT` 后没重新 moc |
| 中文乱码 | 所有源文件存 **UTF-8 无 BOM**；`.pro` 里 `QMAKE_CXXFLAGS += /utf-8` |
| 改了 `.ui` 界面没变 | VS 里要重新生成工程；`ui_*.h` 是生成物不要去改 |
| 数据库里没数据 / 表都没建 | 看 exe 旁边的 `data/app.db` 是不是真的在；调试时的工作目录和 exe 目录可能不同 |
| 两个人都改了同一行，冲突了 | 别慌，`git status` 看哪些文件，商量谁来改，改完 `git add` → `git commit` |
