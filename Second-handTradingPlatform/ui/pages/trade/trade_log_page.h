#pragma once

#include <QWidget>

class TradeService;
class QLineEdit;
class QComboBox;
class QTableWidget;

// 交易记录页（主窗口第 4 个导航页）：全平台流水，可按订单号/状态筛选。
// 只负责显示，业务在 TradeService；Service 指针由 MainWindow 构造时注入，本类不 new 任何东西。
class TradeLogPage : public QWidget
{
    Q_OBJECT

public:
    explicit TradeLogPage(TradeService* service, QWidget* parent = nullptr);

    // MainWindow::onDataChanged 会调它：重新查一次数据，不做增量刷新
    void refresh();

private:
    void doQuery();

    TradeService* m_service = nullptr;
    QLineEdit*    m_keywordEdit = nullptr;
    QComboBox*    m_statusCombo = nullptr;
    QTableWidget* m_table = nullptr;
};
