#pragma once
#include <QWidget>

class OrderService;
class TradeService;
class QComboBox;
class QLineEdit;
class QTableWidget;

// 我的订单：买到的（asSeller=false）和卖出的（asSeller=true）共用一个类。
class MyOrdersPage : public QWidget
{
    Q_OBJECT

public:
    MyOrdersPage(OrderService* orderService, TradeService* tradeService,
                 int userId, bool asSeller, QWidget* parent = nullptr);

    // MainWindow::onDataChanged 会调
    void refresh();
    void setUserId(int userId);

private:
    void openDetail(int row);

    OrderService* m_orderService = nullptr;
    TradeService* m_tradeService = nullptr;
    int           m_userId  = 0;
    bool          m_asSeller = false;

    QComboBox*    m_statusBox   = nullptr;
    QLineEdit*    m_keywordEdit = nullptr;
    QTableWidget* m_table       = nullptr;
};
