#include "ui/mainwindow.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QDate>
#include <QDialog>
#include <QHash>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "core/common/constants.h"
#include "core/entity/product.h"
#include "core/entity/user.h"
#include "core/session.h"

#include "data/db_manager.h"
#include "data/user_dao.h"
#include "data/product_dao.h"
#include "data/order_dao.h"
#include "data/trade_log_dao.h"

#include "services/auth_service.h"
#include "services/product_service.h"
#include "services/order_service.h"
#include "services/trade_service.h"
#include "services/import_service.h"

// 页面
#include "ui/pages/product/search_page.h"
#include "ui/pages/product/my_products_page.h"
#include "ui/pages/product/publish_dialog.h"
#include "ui/pages/order/buy_dialog.h"
#include "ui/pages/order/my_orders_page.h"
#include "ui/pages/trade/trade_log_page.h"
#include "ui/pages/import/import_page.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(QStringLiteral("二手交易平台"));
    resize(1100, 700);

    buildServices();     // 先把对象造好
    buildUi();           // 再造页面（页面构造需要 Service 指针）
    connectBus();        // 最后接线

    statusBar()->showMessage(QStringLiteral("数据库：") + DbManager::dbPath());
}

MainWindow::~MainWindow()
{
    // Service 和页面都是 QObject，parent = this，Qt 会自己删；
    // 四个 DAO 不是 QObject，得在这里手动删
    delete m_importService;  m_importService = nullptr;
    delete m_tradeService;   m_tradeService = nullptr;
    delete m_orderService;   m_orderService = nullptr;
    delete m_productService; m_productService = nullptr;
    delete m_authService;    m_authService = nullptr;

    delete m_logRepo;     m_logRepo = nullptr;
    delete m_orderRepo;   m_orderRepo = nullptr;
    delete m_productRepo; m_productRepo = nullptr;
    delete m_userRepo;    m_userRepo = nullptr;

    delete ui;
}

// ---------------------------------------------------------------- 服务装配

void MainWindow::buildServices()
{
    // 顺序不能错：先 DAO，再 Service（Service 构造要拿到 DAO 指针）
    // 注意：四个 DAO 都没有继承 QObject，构造函数不接 parent，所以这里不能传 this
    m_userRepo    = new UserDao();
    m_productRepo = new ProductDao();
    m_orderRepo   = new OrderDao();
    m_logRepo     = new TradeLogDao();

    m_authService    = new AuthService(m_userRepo, this);
    m_productService = new ProductService(m_productRepo, this);
    m_orderService   = new OrderService(m_orderRepo, m_productRepo, m_logRepo, this);
    m_tradeService   = new TradeService(m_orderRepo, m_productRepo, m_logRepo, this);
    m_importService  = new ImportService(m_productRepo, m_userRepo, this);
}

// ---------------------------------------------------------------- 界面装配

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
    // 0) 先清掉 Designer 里残留的空页（.ui 里曾经有 page / page_2 两个空 QWidget）。
    //    不清的话，代码 addWidget 的页面会整体后移，点第 N 项导航看到的是别的页面。
    while (ui->stackedWidget->count() > 0) {
        QWidget* w = ui->stackedWidget->widget(0);
        ui->stackedWidget->removeWidget(w);
        delete w;
    }

    // 1) 造页面（「我的订单」是 B 的页面，一页两标签：我买到的 / 我卖出的）
    m_searchPage     = new SearchPage(m_productService, this);
    m_myProductsPage = new MyProductsPage(m_productService, this);
    m_tradeLogPage   = new TradeLogPage(m_tradeService, this);
    m_importPage     = new ImportPage(m_importService, this);

    // 用户 id 在构造时还不知道（登录在 main.cpp 里先做完再 new MainWindow 也是 0），
    // 所以先传 0，等 setCurrentUser() 时再用 setUserId 补上。
    m_myOrdersPage = new MyOrdersPage(m_orderService, m_tradeService, 0, false, this);
    m_soldOrdersPage = new MyOrdersPage(m_orderService, m_tradeService, 0, true, this);

    auto* ordersTab = new QTabWidget(this);
    ordersTab->addTab(m_myOrdersPage,   QStringLiteral("我买到的"));
    ordersTab->addTab(m_soldOrdersPage, QStringLiteral("我卖出的"));

    // 2) 按导航顺序放进 QStackedWidget（顺序必须和下面 addItem 一致！）
    ui->stackedWidget->addWidget(m_searchPage);
    ui->stackedWidget->addWidget(m_myProductsPage);
    ui->stackedWidget->addWidget(ordersTab);
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

    // 3) 菜单（.ui 里没有菜单栏，这里用代码建，省得改 xml）
    QMenu* toolMenu = menuBar()->addMenu(QStringLiteral("工具"));
    QAction* resetAct = toolMenu->addAction(QStringLiteral("重置演示数据"));
    connect(resetAct, &QAction::triggered, this, &MainWindow::onResetDemoData);
}

// ---------------------------------------------------------------- 信号总线

void MainWindow::connectBus()
{
    // 页面 → 主窗口：检索页双击商品，交给主窗口去弹购买框
    connect(m_searchPage, &SearchPage::buyRequested,
            this, &MainWindow::onBuyRequested);
    connect(m_searchPage, &SearchPage::publishRequested,
            this, &MainWindow::onPublishRequested);

    // 服务 → 主窗口：任何数据变了，让各列表页各重查一次
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

// ---------------------------------------------------------------- 槽

void MainWindow::onDataChanged()
{
    // 不做增量刷新：谁的数据可能变了就整页重查一次，几十条数据无感
    if (m_searchPage)     m_searchPage->refresh();
    if (m_myProductsPage) m_myProductsPage->refresh();
    if (m_tradeLogPage)   m_tradeLogPage->refresh();
    if (m_myOrdersPage)   m_myOrdersPage->refresh();
    if (m_soldOrdersPage) m_soldOrdersPage->refresh();
}

void MainWindow::onPublishRequested()
{
    if (!Session::instance().isLoggedIn()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先登录"));
        return;
    }
    PublishDialog dlg(m_productService, Session::instance().currentUser().id, this);
    if (dlg.exec() == QDialog::Accepted)
        onDataChanged();
}

void MainWindow::onBuyRequested(int productId)
{
    if (!Session::instance().isLoggedIn()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先登录"));
        return;
    }

    const auto p = m_productService->find(productId);
    if (!p) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("商品不存在，可能刚被别人买走了"));
        onDataChanged();
        return;
    }

    // 购买确认框是 B 的 BuyDialog，下单仍然走 OrderService::createOrder（事务三步）
    BuyDialog dlg(m_orderService, m_productService, productId,
                  Session::instance().currentUser().id, this);
    if (dlg.exec() != QDialog::Accepted)
        return;   // 用户取消，或下单失败（对话框里已经弹过提示）

    statusBar()->showMessage(QStringLiteral("下单成功"), 3000);
    onDataChanged();
}

void MainWindow::onLoginChanged()
{
    if (Session::instance().isLoggedIn())
        setWindowTitle(QStringLiteral("二手交易平台 - %1")
                           .arg(Session::instance().currentUser().nickname));
    else
        setWindowTitle(QStringLiteral("二手交易平台"));
}

void MainWindow::setCurrentUser(const User& user)
{
    Session::instance().setCurrentUser(user);

    // 订单页构造时还不知道用户是谁，这里补上，否则查出来是空表
    if (m_myOrdersPage)   m_myOrdersPage->setUserId(user.id);
    if (m_soldOrdersPage) m_soldOrdersPage->setUserId(user.id);

    onLoginChanged();
    onDataChanged();      // 登录后「我的商品」「我的订单」才有东西可查
}

// ---------------------------------------------------------------- 演示数据

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
                { QString(),                      Const::OrderStatus::PendingPay },
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
                             QStringLiteral("新增商品 %1 件。演示账号：alice / bob / carol，"
                                            "密码都是 123456").arg(inserted));
}
