#pragma once

#include <QMainWindow>

struct User;

class IUserRepository;   class IProductRepository;
class IOrderRepository;  class ITradeLogRepository;
class AuthService;  class ProductService;  class OrderService;  class TradeService;
class ImportService;
class SearchPage;   class MyProductsPage;  class MyOrdersPage;
class TradeLogPage; class ImportPage;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 主窗口 = 组合根 + 中介者：全项目只有这里 new DAO / Service，
// 也只有这里 connect 信号。页面只拿到 Service 指针，不许自己 new 任何数据库对象。
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // 登录成功后由 main.cpp 调用，标题栏显示昵称
    void setCurrentUser(const User& user);

private slots:
    void onDataChanged();                 // 任何数据变了 → 各页面整页重查
    void onBuyRequested(int productId);   // 检索页双击商品
    void onPublishRequested();            // 检索页「发布商品」
    void onLoginChanged();                // 登录 / 退出
    void onResetDemoData();               // 工具 → 重置演示数据

private:
    void buildServices();                 // 建 4 个 DAO + 5 个 Service，全项目唯一 new 的地方
    void buildUi();                       // 建导航项、放页面、建菜单
    void connectBus();                    // 全项目唯一 connect 的地方

    QWidget* makePlaceholder(const QString& text);   // 占位页

    // 四个 DAO / 五个 Service，全部由 MainWindow 持有
    IUserRepository*     m_userRepo    = nullptr;
    IProductRepository*  m_productRepo = nullptr;
    IOrderRepository*    m_orderRepo   = nullptr;
    ITradeLogRepository* m_logRepo     = nullptr;

    AuthService*    m_authService    = nullptr;
    ProductService* m_productService = nullptr;
    OrderService*   m_orderService   = nullptr;
    TradeService*   m_tradeService   = nullptr;
    ImportService*  m_importService  = nullptr;

    // 五个页面
    SearchPage*     m_searchPage     = nullptr;
    MyProductsPage* m_myProductsPage = nullptr;
    MyOrdersPage*   m_myOrdersPage   = nullptr;   // 「我的订单」页里的「我买到的」标签
    MyOrdersPage*   m_soldOrdersPage = nullptr;   // 「我的订单」页里的「我卖出的」标签
    TradeLogPage*   m_tradeLogPage   = nullptr;
    ImportPage*     m_importPage     = nullptr;

    Ui::MainWindow* ui;
};
