#pragma once
#include <QDialog>

class OrderService;
class TradeService;
class QTableWidget;

// 订单详情：订单信息 + 能做的动作（由状态机给）+ 状态日志。
class OrderDetailDialog : public QDialog
{
    Q_OBJECT

public:
    OrderDetailDialog(OrderService* orderService, TradeService* tradeService,
                      int orderId, QWidget* parent = nullptr);

private:
    void applyAction(const QString& action);
    void fillLogs();

    OrderService* m_orderService = nullptr;
    TradeService* m_tradeService = nullptr;
    int           m_orderId = 0;

    QTableWidget* m_logTable = nullptr;
};
