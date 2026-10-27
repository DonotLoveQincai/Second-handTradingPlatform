#include "ui/pages/trade/trade_log_page.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "core/common/constants.h"
#include "core/entity/trade_log.h"
#include "services/trade_service.h"
#include "services/trade_state_machine.h"

TradeLogPage::TradeLogPage(TradeService* service, QWidget* parent)
    : QWidget(parent)
    , m_service(service)
{
    // —— 顶部筛选条：订单号 / 状态 / 查询 ——
    m_keywordEdit = new QLineEdit(this);
    m_keywordEdit->setPlaceholderText(QStringLiteral("订单号 / 操作人 / 备注"));

    m_statusCombo = new QComboBox(this);
    m_statusCombo->addItem(QStringLiteral("全部"), QString());                        // 空串 = 不限
    m_statusCombo->addItem(QStringLiteral("待付款"), Const::OrderStatus::PendingPay);
    m_statusCombo->addItem(QStringLiteral("已付款"), Const::OrderStatus::Paid);
    m_statusCombo->addItem(QStringLiteral("已发货"), Const::OrderStatus::Shipped);
    m_statusCombo->addItem(QStringLiteral("已完成"), Const::OrderStatus::Completed);
    m_statusCombo->addItem(QStringLiteral("已取消"), Const::OrderStatus::Cancelled);

    auto* searchBtn = new QPushButton(QStringLiteral("查询"), this);

    auto* filterBar = new QHBoxLayout;
    filterBar->addWidget(new QLabel(QStringLiteral("订单号"), this));
    filterBar->addWidget(m_keywordEdit, 1);
    filterBar->addWidget(new QLabel(QStringLiteral("状态"), this));
    filterBar->addWidget(m_statusCombo);
    filterBar->addWidget(searchBtn);

    // —— 结果表格：只读，禁止编辑 ——
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("时间"), QStringLiteral("订单号"), QStringLiteral("状态变化"),
        QStringLiteral("动作"), QStringLiteral("操作人"), QStringLiteral("备注"),
    });
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->horizontalHeader()->setStretchLastSection(true);

    auto* root = new QVBoxLayout(this);
    root->addLayout(filterBar);
    root->addWidget(m_table, 1);

    connect(searchBtn, &QPushButton::clicked, this, &TradeLogPage::doQuery);
}

void TradeLogPage::refresh()
{
    doQuery();
}

void TradeLogPage::doQuery()
{
    TradeLogQuery q;
    q.keyword = m_keywordEdit->text().trimmed();
    q.status  = m_statusCombo->currentData().toString();   // 下拉里存的是英文常量，"全部"是空串

    const QVector<TradeLog> logs = m_service->queryLogs(q);

    m_table->setRowCount(logs.size());
    for (int i = 0; i < logs.size(); ++i) {
        const TradeLog& l = logs.at(i);
        const QString change = l.fromStatus.isEmpty()
            ? QStringLiteral("— → %1").arg(Const::zh(l.toStatus))
            : QStringLiteral("%1 → %2").arg(Const::zh(l.fromStatus), Const::zh(l.toStatus));
        m_table->setItem(i, 0, new QTableWidgetItem(l.createdAt));
        m_table->setItem(i, 1, new QTableWidgetItem(l.orderNo));
        m_table->setItem(i, 2, new QTableWidgetItem(change));
        m_table->setItem(i, 3, new QTableWidgetItem(TradeStateMachine::actionName(l.action)));
        m_table->setItem(i, 4, new QTableWidgetItem(l.operatorName));
        m_table->setItem(i, 5, new QTableWidgetItem(l.remark));
    }
}
