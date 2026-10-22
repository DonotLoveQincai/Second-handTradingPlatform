# 二手寄售交易平台

课程设计作品，运行在 Windows 上的桌面程序。使用 C++ 和 Qt Widgets 开发，界面用 Qt 制作，数据保存在本地 SQLite 数据库文件中，运行时不联网。

## 功能

- 商品：寄售商品上架、修改信息、下架，按分类和关键字检索。
- 买家：注册登记，下单购买，查看自己的订单记录。
- 交易：订单按待付款、已付款、已发货、已完成、已取消五种状态流转，每次变更写入交易记录，可以按关键字、状态和日期查询。
- 界面：主窗口包含五个功能页面，以及登录注册、发布商品、订单详情等对话框，并支持从 CSV 文件批量导入商品。

## 分工

| 成员 | 负责模块 | 主要内容 |
| --- | --- | --- |
| 海天宇 | 商品模块 | 商品上架、修改、下架、分类检索，对应 `core/entity/product.h`、`data/product_dao.*`、`services/product_service.*`、`ui/pages/product/` |
| 段骁越 | 用户与订单模块 | 注册登录、下单、订单记录，对应 `core/entity/user.h`、`core/entity/order.h`、`data/user_dao.*`、`data/order_dao.*`、`services/auth_service.*`、`services/order_service.*`、`ui/pages/auth/`、`ui/pages/order/` |
| 张煊熙 | 交易状态模块 | 状态流转规则、变更记录、记录查询，对应 `core/entity/trade_log.h`、`services/trade_state_machine.*`、`services/trade_service.*`、`data/trade_log_dao.*`、`ui/pages/trade/` |
| 张宸宇 | 平台底座与界面整合 | 工程结构、数据库初始化、主窗口组装、批量导入，对应 `main.cpp`、`.pro` 与 `.pri`、`data/db_manager.*`、`sql/`、`ui/mainwindow.*`、`services/import_service.*` |

四人按业务模块分工，各自维护一个 Git 分支，模块之间通过 `core/repository/` 中的接口调用，主窗口的组装和分支合并由 D 负责。

## 运行

1. 安装 Visual Studio 2022（勾选“使用 C++ 的桌面开发”）和 Qt 6.11.2（MSVC 2022 64 位），并在 Visual Studio 中安装 Qt VS Tools 扩展。
2. 用 Visual Studio 打开 `Second-handTradingPlatform.sln`，选择 Debug x64，按 F5 运行。
3. 首次运行自动创建数据库，文件位于程序目录下的 `data/app.db`。
4. 程序菜单中有“重置演示数据”，可生成三个演示账号 alice、bob、carol（密码均为 123456）和若干件在售商品。

## 文档

- [docs/01-契约.md](docs/01-契约.md)：数据库表结构、接口定义、交易状态机。
- [docs/02-环境与协作手册.md](docs/02-环境与协作手册.md)：环境配置、Git 使用流程、常见问题。

