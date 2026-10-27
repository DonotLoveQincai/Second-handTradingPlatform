// 联调用例：不启动界面，直接调 services + DAO，跑真实 SQLite。
//
// 用法：
//   1) 在 tests 目录里 qmake -tp vc 或直接 qmake + nmake
//   2) 双击 tests/debug/integration_test.exe
//   3) 看控制台输出：每行 PASS / FAIL，最后一行是汇总；有任何 FAIL 就返回码 1
//
// 每跑一次都会删掉 tests 目录下的 data/app.db，从空库开始，可反复跑。

#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include "core/common/constants.h"
#include "core/common/opresult.h"
#include "core/entity/order.h"
#include "core/entity/product.h"
#include "core/entity/trade_log.h"
#include "core/entity/user.h"
#include "core/session.h"

#include "data/db_manager.h"
#include "data/order_dao.h"
#include "data/product_dao.h"
#include "data/trade_log_dao.h"
#include "data/user_dao.h"

#include "services/auth_service.h"
#include "services/import_service.h"
#include "services/order_service.h"
#include "services/product_service.h"
#include "services/trade_service.h"
#include "services/trade_state_machine.h"

static int g_pass = 0;
static int g_fail = 0;

static void check(bool cond, const QString& what, const QString& detail = QString())
{
    if (cond) {
        ++g_pass;
        QTextStream(stdout) << "PASS  " << what << "\n";
    } else {
        ++g_fail;
        QTextStream(stdout) << "FAIL  " << what;
        if (!detail.isEmpty())
            QTextStream(stdout) << "   [" << detail << "]";
        QTextStream(stdout) << "\n";
    }
}

static void section(const QString& title)
{
    QTextStream(stdout) << "\n---- " << title << " ----\n";
}

// 某商品当前状态（直接查库，不信缓存）
static QString productStatus(IProductRepository* repo, int productId)
{
    const auto p = repo->findById(productId);
    return p ? p->status : QStringLiteral("<不存在>");
}

static int orderCount(IOrderRepository* repo, int buyerId)
{
    OrderQuery q;
    q.buyerId = buyerId;
    return repo->search(q).size();
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    // ---------- 0) 干净的空库（DbManager 用的是 applicationDirPath()/data/app.db） ----------
    const QString dataDir = QCoreApplication::applicationDirPath() + QStringLiteral("/data");
    QDir().mkpath(dataDir);
    QFile::remove(dataDir + QStringLiteral("/app.db"));
    QFile::remove(dataDir + QStringLiteral("/app.db-wal"));
    QFile::remove(dataDir + QStringLiteral("/app.db-shm"));

    QString err;
    check(DbManager::init(&err), QStringLiteral("0. 建库成功"), err);

    // ---------- 1) 组装（和 MainWindow::buildServices 一样的顺序） ----------
    UserDao    userDao;
    ProductDao productDao;
    OrderDao   orderDao;
    TradeLogDao logDao;

    AuthService    auth(&userDao);
    ProductService productSvc(&productDao);
    OrderService   orderSvc(&orderDao, &productDao, &logDao);
    TradeService   tradeSvc(&orderDao, &productDao, &logDao);
    ImportService  importSvc(&productDao, &userDao);

    section(QStringLiteral("1. 验收清单第 0 步：状态机自检"));
    TradeStateMachine::selfCheck();   // 内部 Q_ASSERT：20 组合 × 5 条合法边，不通过会直接崩
    check(true, QStringLiteral("1.1 selfCheck 跑完没触发断言（5 状态 × 4 动作 = 20 组合）"));

    section(QStringLiteral("2. 注册 / 登录（功能②买家寄售登记）"));
    auto r1 = auth.registerUser(QStringLiteral("alice"), QStringLiteral("Alice"),
                                QStringLiteral("123456"), QStringLiteral("123456"));
    check(r1.ok, QStringLiteral("2.1 注册 alice"), r1.message);
    auto r2 = auth.registerUser(QStringLiteral("bob"), QStringLiteral("Bob"),
                                QStringLiteral("123456"), QStringLiteral("123456"));
    check(r2.ok, QStringLiteral("2.2 注册 bob"), r2.message);

    auto dup = auth.registerUser(QStringLiteral("alice"), QStringLiteral("Alice2"),
                                 QStringLiteral("123456"), QStringLiteral("123456"));
    check(!dup.ok, QStringLiteral("2.3 重名注册被拒"), dup.message);

    auto shortPwd = auth.registerUser(QStringLiteral("carol"), QStringLiteral("Carol"),
                                      QStringLiteral("123"), QStringLiteral("123"));
    check(!shortPwd.ok, QStringLiteral("2.4 密码太短被拒"), shortPwd.message);

    auto mismatch = auth.registerUser(QStringLiteral("carol"), QStringLiteral("Carol"),
                                      QStringLiteral("123456"), QStringLiteral("654321"));
    check(!mismatch.ok, QStringLiteral("2.5 两次密码不一致被拒"), mismatch.message);

    const auto aliceOpt = userDao.findByUsername(QStringLiteral("alice"));
    const auto bobOpt   = userDao.findByUsername(QStringLiteral("bob"));
    check(aliceOpt && bobOpt, QStringLiteral("2.6 能按用户名查回 alice / bob"));
    if (!aliceOpt || !bobOpt)
        return 1;
    const int aliceId = aliceOpt->id;
    const int bobId   = bobOpt->id;

    auto badLogin = auth.login(QStringLiteral("alice"), QStringLiteral("000000"));
    check(!badLogin.ok, QStringLiteral("2.7 密码错登录失败"), badLogin.message);
    auto goodLogin = auth.login(QStringLiteral("alice"), QStringLiteral("123456"));
    check(goodLogin.ok, QStringLiteral("2.8 正确密码登录成功"), goodLogin.message);
    check(Session::instance().isLoggedIn(), QStringLiteral("2.9 登录后 Session 里有当前用户"));
    auth.logout();
    check(!Session::instance().isLoggedIn(), QStringLiteral("2.10 退出后 Session 清空"));

    section(QStringLiteral("3. 分类：从 categories 表读（不是写死的）"));
    const auto cats = productSvc.categories();
    check(cats.size() == 8, QStringLiteral("3.1 分类 8 条"),
          QStringLiteral("实际 %1").arg(cats.size()));
    int parents = 0, children = 0;
    for (const auto& c : cats) {
        if (c.parentId == 0) ++parents; else ++children;
    }
    check(parents == 5 && children == 3, QStringLiteral("3.2 5 个父类 + 3 个子类"),
          QStringLiteral("实际 %1/%2").arg(parents).arg(children));

    section(QStringLiteral("4. 商品上架（功能①）"));
    Product p1;
    p1.title       = QStringLiteral("九成新 iPhone 13 128G");
    p1.description = QStringLiteral("国行，电池 92%");
    p1.price       = 3200.0;
    p1.categoryId  = 6;     // 手机
    p1.conditionLevel = 9;
    auto pub1 = productSvc.publish(p1, aliceId);
    check(pub1.ok, QStringLiteral("4.1 alice 发布商品"), pub1.message);
    check(p1.id > 0, QStringLiteral("4.2 发布后 id 被回填"));
    check(p1.status == Const::ProductStatus::OnSale, QStringLiteral("4.3 新商品状态是在售"), p1.status);

    Product p2;
    p2.title = QStringLiteral("二手 ThinkPad T480");
    p2.price = 2100.0;
    p2.categoryId = 7;      // 电脑
    auto pub2 = productSvc.publish(p2, aliceId);
    check(pub2.ok, QStringLiteral("4.4 再发布一件"), pub2.message);

    Product bad;
    bad.title = QStringLiteral("");
    bad.price = 10.0;
    bad.categoryId = 6;
    auto pubBad = productSvc.publish(bad, aliceId);
    check(!pubBad.ok, QStringLiteral("4.5 空标题被拒"), pubBad.message);

    Product bad2;
    bad2.title = QStringLiteral("白送的东西");
    bad2.price = 0.0;
    bad2.categoryId = 6;
    auto pubBad2 = productSvc.publish(bad2, aliceId);
    check(!pubBad2.ok, QStringLiteral("4.6 价格 <= 0 被拒"), pubBad2.message);

    Product bad3;
    bad3.title = QStringLiteral("没分类的东西");
    bad3.price = 10.0;
    auto pubBad3 = productSvc.publish(bad3, aliceId);
    check(!pubBad3.ok, QStringLiteral("4.7 没选分类被拒"), pubBad3.message);

    section(QStringLiteral("5. 商品修改 / 下架 / 上架（功能①的信息修改）"));
    auto p1read = productDao.findById(p1.id);
    check(p1read.has_value(), QStringLiteral("5.1 能按 id 查回商品"));
    if (p1read) {
        Product edit = *p1read;
        edit.price = 2999.0;
        edit.description = QStringLiteral("降价了，国行，电池 92%");
        auto up = productSvc.updateInfo(edit, aliceId);
        check(up.ok, QStringLiteral("5.2 alice 改自己商品的价格"), up.message);
        auto after = productDao.findById(p1.id);
        check(after && qFuzzyCompare(after->price, 2999.0),
              QStringLiteral("5.3 价格真的改成了 2999"),
              after ? QString::number(after->price) : QStringLiteral("<查不到>"));
    }

    if (p1read) {
        Product edit = *p1read;
        edit.price = 1.0;
        auto notMine = productSvc.updateInfo(edit, bobId);
        check(!notMine.ok, QStringLiteral("5.4 bob 改不了 alice 的商品"), notMine.message);
    }

    auto td = productSvc.takeDown(p1.id, aliceId);
    check(td.ok, QStringLiteral("5.5 下架自己的商品"), td.message);
    check(productStatus(&productDao, p1.id) == Const::ProductStatus::OffSale,
          QStringLiteral("5.6 下架后状态是 OFF_SALE"), productStatus(&productDao, p1.id));

    auto tdOther = productSvc.takeDown(p2.id, bobId);
    check(!tdOther.ok, QStringLiteral("5.7 bob 下架不了 alice 的商品"), tdOther.message);

    auto po = productSvc.putOnSale(p1.id, aliceId);
    check(po.ok, QStringLiteral("5.8 重新上架"), po.message);
    check(productStatus(&productDao, p1.id) == Const::ProductStatus::OnSale,
          QStringLiteral("5.9 上架后状态是 ON_SALE"), productStatus(&productDao, p1.id));

    section(QStringLiteral("6. 分类检索（功能①）"));
    ProductQuery q;
    q.keyword = QStringLiteral("iPhone");
    auto found = productSvc.search(q);
    check(found.size() == 1, QStringLiteral("6.1 关键字 iPhone 命中 1 件"),
          QStringLiteral("实际 %1").arg(found.size()));

    ProductQuery q2;
    q2.categoryIds = { 6 };
    auto byCat = productSvc.search(q2);
    check(byCat.size() >= 1, QStringLiteral("6.2 按分类「手机」能查到"),
          QStringLiteral("实际 %1").arg(byCat.size()));

    ProductQuery q3;
    q3.priceMin = 2000;
    q3.priceMax = 3100;
    auto byPrice = productSvc.search(q3);
    check(byPrice.size() >= 2, QStringLiteral("6.3 价格区间 2000~3100 命中 iPhone 与 ThinkPad"),
          QStringLiteral("实际 %1").arg(byPrice.size()));

    ProductQuery q4;
    q4.keyword = QStringLiteral("完全不存在的商品");
    auto none = productSvc.search(q4);
    check(none.isEmpty(), QStringLiteral("6.4 查不到时返回空"),
          QStringLiteral("实际 %1").arg(none.size()));

    auto mine = productSvc.myProducts(aliceId);
    check(mine.size() == 2, QStringLiteral("6.5 alice 的「我的商品」有 2 件"),
          QStringLiteral("实际 %1").arg(mine.size()));

    section(QStringLiteral("7. 下单：跨域事务三步（功能②）"));
    const int ordersBefore = orderCount(&orderDao, bobId);
    int orderId = 0;
    auto create = orderSvc.createOrder(p1.id, bobId, QStringLiteral("周末面交"), &orderId);
    check(create.ok, QStringLiteral("7.1 bob 买 alice 的商品"), create.message);
    check(orderId > 0, QStringLiteral("7.2 拿到订单 id"));
    check(productStatus(&productDao, p1.id) == Const::ProductStatus::Sold,
          QStringLiteral("7.3 商品被置为 SOLD（事务第 2 步）"), productStatus(&productDao, p1.id));
    check(orderCount(&orderDao, bobId) == ordersBefore + 1,
          QStringLiteral("7.4 订单表多了一条（事务第 1 步）"));

    const auto logs1 = tradeSvc.orderLogs(orderId);
    check(logs1.size() == 1, QStringLiteral("7.5 第一条状态日志已写（事务第 3 步）"),
          QStringLiteral("实际 %1 条").arg(logs1.size()));
    if (!logs1.isEmpty()) {
        check(logs1.first().fromStatus.isEmpty()
                  && logs1.first().toStatus == Const::OrderStatus::PendingPay
                  && logs1.first().action == Const::TradeAction::Create,
              QStringLiteral("7.6 首条日志 = \"\" → PENDING_PAY / CREATE"),
              logs1.first().fromStatus + QStringLiteral(" → ") + logs1.first().toStatus);
    }

    const auto order1 = orderDao.findById(orderId);
    check(order1.has_value(), QStringLiteral("7.7 能查到订单详情"));
    if (order1) {
        check(qFuzzyCompare(order1->amount, 2999.0),
              QStringLiteral("7.8 订单金额 = 商品当前价 2999"),
              QString::number(order1->amount));
        check(order1->orderNo.startsWith(QDate::currentDate().toString(QStringLiteral("yyyyMMdd"))),
              QStringLiteral("7.9 订单号以日期开头"), order1->orderNo);
        check(order1->status == Const::OrderStatus::PendingPay,
              QStringLiteral("7.10 订单初始状态 PENDING_PAY"), order1->status);
    }

    section(QStringLiteral("8. 连点两次 / 自己买自己的东西"));
    int orderId2 = 0;
    auto again = orderSvc.createOrder(p1.id, bobId, QString(), &orderId2);
    check(!again.ok, QStringLiteral("8.1 同一件再买一次被拦"), again.message);
    check(orderCount(&orderDao, bobId) == ordersBefore + 1,
          QStringLiteral("8.2 没有生成第二张订单"),
          QStringLiteral("实际 %1 条").arg(orderCount(&orderDao, bobId)));
    check(again.message.contains(QStringLiteral("已售出")),
          QStringLiteral("8.3 提示是「已售出或已下架」这条"), again.message);

    int selfOrder = 0;
    auto selfBuy = orderSvc.createOrder(p2.id, aliceId, QString(), &selfOrder);
    check(!selfBuy.ok, QStringLiteral("8.4 自己不能买自己的商品"), selfBuy.message);

    section(QStringLiteral("9. 状态流转：付款 → 发货 → 确认收货（功能③）"));
    auto payBySeller = tradeSvc.changeStatus(orderId, Const::TradeAction::Pay, aliceId);
    check(!payBySeller.ok, QStringLiteral("9.1 卖家不能替买家付款"), payBySeller.message);

    auto pay = tradeSvc.changeStatus(orderId, Const::TradeAction::Pay, bobId);
    check(pay.ok, QStringLiteral("9.2 买家付款"), pay.message);
    auto o2 = orderDao.findById(orderId);
    check(o2 && o2->status == Const::OrderStatus::Paid, QStringLiteral("9.3 状态变 PAID"),
          o2 ? o2->status : QString());

    auto payAgain = tradeSvc.changeStatus(orderId, Const::TradeAction::Pay, bobId);
    check(!payAgain.ok, QStringLiteral("9.4 已付款再付一次被状态机拒"), payAgain.message);

    auto shipByBuyer = tradeSvc.changeStatus(orderId, Const::TradeAction::Ship, bobId);
    check(!shipByBuyer.ok, QStringLiteral("9.5 买家不能发货"), shipByBuyer.message);

    auto ship = tradeSvc.changeStatus(orderId, Const::TradeAction::Ship, aliceId);
    check(ship.ok, QStringLiteral("9.6 卖家发货"), ship.message);

    auto confirmBySeller = tradeSvc.changeStatus(orderId, Const::TradeAction::Confirm, aliceId);
    check(!confirmBySeller.ok, QStringLiteral("9.7 卖家不能替买家确认收货"), confirmBySeller.message);

    auto confirm = tradeSvc.changeStatus(orderId, Const::TradeAction::Confirm, bobId);
    check(confirm.ok, QStringLiteral("9.8 买家确认收货"), confirm.message);

    auto o3 = orderDao.findById(orderId);
    check(o3 && o3->status == Const::OrderStatus::Completed, QStringLiteral("9.9 终态 COMPLETED"),
          o3 ? o3->status : QString());

    const auto logs2 = tradeSvc.orderLogs(orderId);
    check(logs2.size() == 4, QStringLiteral("9.10 一单走到完成共 4 条日志"),
          QStringLiteral("实际 %1 条").arg(logs2.size()));
    if (logs2.size() == 4) {
        QString seq;
        for (const auto& l : logs2)
            seq += (seq.isEmpty() ? QString() : QStringLiteral(" → ")) + l.toStatus;
        check(logs2.at(0).toStatus == Const::OrderStatus::PendingPay
                  && logs2.at(1).toStatus == Const::OrderStatus::Paid
                  && logs2.at(2).toStatus == Const::OrderStatus::Shipped
                  && logs2.at(3).toStatus == Const::OrderStatus::Completed,
              QStringLiteral("9.11 日志顺序 = 待付款 → 已付款 → 已发货 → 已完成（按 id 正序）"),
              QStringLiteral("实际 %1").arg(seq));
    }

    auto cancelDone = tradeSvc.changeStatus(orderId, Const::TradeAction::Cancel, bobId);
    check(!cancelDone.ok, QStringLiteral("9.12 已完成的订单不能取消"), cancelDone.message);

    section(QStringLiteral("10. 取消订单 → 商品回到在售（事务第 3 步的 Cancel 分支）"));
    int orderId3 = 0;
    auto create3 = orderSvc.createOrder(p2.id, bobId, QStringLiteral("试试取消"), &orderId3);
    check(create3.ok, QStringLiteral("10.1 bob 买第二件"), create3.message);
    check(productStatus(&productDao, p2.id) == Const::ProductStatus::Sold,
          QStringLiteral("10.2 商品变 SOLD"), productStatus(&productDao, p2.id));

    auto cancel = tradeSvc.changeStatus(orderId3, Const::TradeAction::Cancel, bobId);
    check(cancel.ok, QStringLiteral("10.3 待付款状态下买家可以取消"), cancel.message);
    auto o4 = orderDao.findById(orderId3);
    check(o4 && o4->status == Const::OrderStatus::Cancelled,
          QStringLiteral("10.4 订单状态 CANCELLED"), o4 ? o4->status : QString());
    check(productStatus(&productDao, p2.id) == Const::ProductStatus::OnSale,
          QStringLiteral("10.5 商品被放回 ON_SALE"),
          productStatus(&productDao, p2.id));

    const auto logs3 = tradeSvc.orderLogs(orderId3);
    check(logs3.size() == 2, QStringLiteral("10.6 取消后共 2 条日志"),
          QStringLiteral("实际 %1 条").arg(logs3.size()));

    // 已付款后：买家不能取消，卖家可以
    int orderId4 = 0;
    auto create4 = orderSvc.createOrder(p2.id, bobId, QString(), &orderId4);
    check(create4.ok, QStringLiteral("10.7 再买一次（商品又回到在售了）"), create4.message);
    tradeSvc.changeStatus(orderId4, Const::TradeAction::Pay, bobId);
    auto cancelPaidByBuyer = tradeSvc.changeStatus(orderId4, Const::TradeAction::Cancel, bobId);
    check(!cancelPaidByBuyer.ok, QStringLiteral("10.8 已付款后买家不能取消"),
          cancelPaidByBuyer.message);
    auto cancelPaidBySeller = tradeSvc.changeStatus(orderId4, Const::TradeAction::Cancel, aliceId);
    check(cancelPaidBySeller.ok, QStringLiteral("10.9 已付款后卖家可以取消"),
          cancelPaidBySeller.message);
    check(productStatus(&productDao, p2.id) == Const::ProductStatus::OnSale,
          QStringLiteral("10.10 取消后商品再次回到在售"),
          productStatus(&productDao, p2.id));

    section(QStringLiteral("11. 交易记录查询（功能③的记录查询）"));
    TradeLogQuery lqAll;
    const auto allLogs = tradeSvc.queryLogs(lqAll);
    check(allLogs.size() >= 8, QStringLiteral("11.1 全平台日志能查出来"),
          QStringLiteral("实际 %1 条").arg(allLogs.size()));

    TradeLogQuery lqCancel;
    lqCancel.status = Const::OrderStatus::Cancelled;
    const auto cancelLogs = tradeSvc.queryLogs(lqCancel);
    check(!cancelLogs.isEmpty() && cancelLogs.size() <= allLogs.size(),
          QStringLiteral("11.2 按状态过滤（已取消）有结果"),
          QStringLiteral("实际 %1 条").arg(cancelLogs.size()));

    // 关键字匹配的是「订单号 / 备注 / 操作人昵称」（见 C 文档 §6 的 SQL），不含商品名
    TradeLogQuery lqKw;
    lqKw.keyword = order1 ? order1->orderNo : QString();
    const auto kwLogs = tradeSvc.queryLogs(lqKw);
    check(!kwLogs.isEmpty(), QStringLiteral("11.3 按订单号关键字能查"),
          QStringLiteral("实际 %1 条").arg(kwLogs.size()));

    TradeLogQuery lqOrder;
    lqOrder.orderId = orderId;
    check(tradeSvc.queryLogs(lqOrder).size() == 4,
          QStringLiteral("11.4 只看某一单的日志 = 4 条"));

    section(QStringLiteral("12. CSV 批量导入（D 的导入引擎）"));
    const QString csvPath = dataDir + QStringLiteral("/it_import_ok.csv");
    {
        QFile f(csvPath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&f);
            ts.setEncoding(QStringConverter::Utf8);
            ts << "title,category,price,condition_level,description\n";
            ts << "罗技 K380 键盘,电脑,89,8,蓝牙连接正常\n";
            ts << "AirPods 3 港版,数码电子 / 耳机,620,8,无明显划痕\n";
        }
    }
    const int beforeImport = productDao.findBySeller(aliceId).size();
    auto rep = importSvc.importCsv(csvPath, aliceId);
    if (!rep.errors.isEmpty())
        QTextStream(stdout) << "      导入错误：" << rep.errors.join(QStringLiteral(" | ")) << "\n";
    check(rep.total == 2 && rep.succeeded == 2 && rep.errors.isEmpty(),
          QStringLiteral("12.1 两行合法数据全部导入"),
          QStringLiteral("total=%1 ok=%2 err=%3").arg(rep.total).arg(rep.succeeded).arg(rep.errors.size()));
    check(productDao.findBySeller(aliceId).size() == beforeImport + 2,
          QStringLiteral("12.2 库里真的多了 2 件"),
          QStringLiteral("导入前 %1，现在 %2")
              .arg(beforeImport).arg(productDao.findBySeller(aliceId).size()));
    check(rep.succeeded == 2, QStringLiteral("12.3 成色/分类都识别了"));

    section(QStringLiteral("13. CSV 里有一行错 → 一条都不入库"));
    const QString csvBad = dataDir + QStringLiteral("/it_import_bad.csv");
    {
        QFile f(csvBad);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&f);
            ts.setEncoding(QStringConverter::Utf8);
            ts << "title,category,price,condition_level\n";
            ts << "好的一行,电脑,99,8\n";
            ts << "价格不对的行,电脑,abc,8\n";
        }
    }
    const int beforeBad = productDao.findBySeller(aliceId).size();
    auto repBad = importSvc.importCsv(csvBad, aliceId);
    check(repBad.total == 2 && repBad.succeeded == 0 && !repBad.errors.isEmpty(),
          QStringLiteral("13.1 有 1 行错误 → 0 行入库、报出错误"),
          QStringLiteral("total=%1 ok=%2 err=%3")
              .arg(repBad.total).arg(repBad.succeeded).arg(repBad.errors.size()));
    check(productDao.findBySeller(aliceId).size() == beforeBad,
          QStringLiteral("13.2 库里一条没多（整批回滚）"),
          QStringLiteral("之前 %1，现在 %2")
              .arg(beforeBad).arg(productDao.findBySeller(aliceId).size()));
    if (!repBad.errors.isEmpty())
        QTextStream(stdout) << "      错误文案：" << repBad.errors.first() << "\n";

    section(QStringLiteral("14. 导入表头不对 / 文件不存在"));
    const QString csvHeader = dataDir + QStringLiteral("/it_import_header.csv");
    {
        QFile f(csvHeader);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&f);
            ts << "a,b,c,d\n1,2,3,4\n";
        }
    }
    auto repH = importSvc.importCsv(csvHeader, aliceId);
    check(repH.succeeded == 0 && !repH.errors.isEmpty(),
          QStringLiteral("14.1 表头不对直接报错"), repH.errors.value(0));

    auto repMissing = importSvc.importCsv(dataDir + QStringLiteral("/no_such_file.csv"), aliceId);
    check(repMissing.succeeded == 0 && !repMissing.errors.isEmpty(),
          QStringLiteral("14.2 文件不存在给出提示"), repMissing.errors.value(0));

    section(QStringLiteral("15. 订单查询（我的订单页用的都是这些）"));
    OrderQuery oqBuyer;
    oqBuyer.buyerId = bobId;
    check(orderSvc.queryOrders(oqBuyer).size() == 3, QStringLiteral("15.1 买家视角订单数 = 3（本次用例一共建了 3 单）"),
          QStringLiteral("实际 %1").arg(orderSvc.queryOrders(oqBuyer).size()));

    OrderQuery oqSeller;
    oqSeller.sellerId = aliceId;
    check(orderSvc.queryOrders(oqSeller).size() == 3, QStringLiteral("15.2 卖家视角订单数 = 3（同一批单，卖家是 alice）"),
          QStringLiteral("实际 %1").arg(orderSvc.queryOrders(oqSeller).size()));

    OrderQuery oqStatus;
    oqStatus.status = Const::OrderStatus::Completed;
    check(!orderSvc.queryOrders(oqStatus).isEmpty(), QStringLiteral("15.3 按状态过滤有结果"));

    check(orderSvc.findOrder(orderId).has_value(), QStringLiteral("15.4 订单详情能查到"));

    OrderQuery oqKw;
    oqKw.keyword = QStringLiteral("iPhone");
    check(!orderSvc.queryOrders(oqKw).isEmpty(), QStringLiteral("15.5 按商品名关键字查订单"));

    // ---------- 汇总 ----------
    QTextStream(stdout) << "\n=====================================\n";
    QTextStream(stdout) << "PASS " << g_pass << "   FAIL " << g_fail << "\n";
    QTextStream(stdout) << "=====================================\n";

    return g_fail == 0 ? 0 : 1;
}
