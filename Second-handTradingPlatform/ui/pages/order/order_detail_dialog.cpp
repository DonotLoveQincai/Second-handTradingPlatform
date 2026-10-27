#include "ui/pages/order/order_detail_dialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "core/common/constants.h"
#include "core/common/opresult.h"
#include "core/entity/order.h"
#include "core/entity/trade_log.h"
#include "core/session.h"
#include "services/order_service.h"
#include "services/trade_service.h"
#include "services/trade_state_machine.h"

OrderDetailDialog::OrderDetailDialog(OrderService* orderService, TradeService* tradeService,
                                     int orderId, QWidget* parent)
    : QDialog(parent)
    , m_orderService(orderService)
    , m_tradeService(tradeService)
    , m_orderId(orderId)
{
    setWindowTitle(QStringLiteral("订单详情"));
    setMinimumSize(640, 480);

    const auto o = m_orderService->findOrder(m_orderId);
    if (!o) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("订单不存在"));
        reject();
        return;
    }

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("订单号"), new QLabel(o->orderNo, this));
    form->addRow(QStringLiteral("商品"),
                 new QLabel(o->productTitle.isEmpty() ? QStringLiteral("商品 #%1").arg(o->productId) : o->productTitle, this));
    form->addRow(QStringLiteral("金额"), new QLabel(QStringLiteral("￥%1").arg(o->amount, 0, 'f', 2), this));
    form->addRow(QStringLiteral("买家"),
                 new QLabel(o->buyerName.isEmpty() ? QStringLiteral("用户 #%1").arg(o->buyerId) : o->buyerName, this));
    form->addRow(QStringLiteral("卖家"),
                 new QLabel(o->sellerName.isEmpty() ? QStringLiteral("用户 #%1").arg(o->sellerId) : o->sellerName, this));
    form->addRow(QStringLiteral("状态"), new QLabel(Const::zh(o->status), this));
    form->addRow(QStringLiteral("备注"), new QLabel(o->remark.isEmpty() ? QStringLiteral("—") : o->remark, this));
    form->addRow(QStringLiteral("创建时间"), new QLabel(o->createdAt, this));

    // 当前状态能做什么，只问状态机，界面不自己判断
    auto* actionBar = new QHBoxLayout;
    const QStringList actions = TradeStateMachine::actionsFor(o->status);
    if (actions.isEmpty()) {
        actionBar->addWidget(new QLabel(QStringLiteral("当前状态没有可执行的操作"), this));
    } else {
        for (const QString& a : actions) {
            auto* btn = new QPushButton(TradeStateMachine::actionName(a), this);
            connect(btn, &QPushButton::clicked, this, [this, a] { applyAction(a); });
            actionBar->addWidget(btn);
        }
    }
    actionBar->addStretch();

    m_logTable = new QTableWidget(this);
    m_logTable->setColumnCount(6);
    m_logTable->setHorizontalHeaderLabels({QStringLiteral("时间"), QStringLiteral("动作"),
                                           QStringLiteral("从"),   QStringLiteral("到"),
                                           QStringLiteral("操作人"), QStringLiteral("备注")});
    m_logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_logTable->verticalHeader()->setVisible(false);
    m_logTable->horizontalHeader()->setStretchLastSection(true);
    fillLogs();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(actionBar);
    layout->addWidget(new QLabel(QStringLiteral("状态日志"), this));
    layout->addWidget(m_logTable);
    layout->addWidget(buttons);
}

void OrderDetailDialog::fillLogs()
{
    // DAO 已按 id 正序返回，这里不再排
    const QVector<TradeLog> logs = m_tradeService->orderLogs(m_orderId);

    m_logTable->setRowCount(logs.size());
    for (int i = 0; i < logs.size(); ++i) {
        const TradeLog& log = logs.at(i);
        m_logTable->setItem(i, 0, new QTableWidgetItem(log.createdAt));
        m_logTable->setItem(i, 1, new QTableWidgetItem(TradeStateMachine::actionName(log.action)));
        m_logTable->setItem(i, 2, new QTableWidgetItem(log.fromStatus.isEmpty()
                                                           ? QStringLiteral("—")
                                                           : Const::zh(log.fromStatus)));
        m_logTable->setItem(i, 3, new QTableWidgetItem(Const::zh(log.toStatus)));
        m_logTable->setItem(i, 4, new QTableWidgetItem(log.operatorName.isEmpty()
                                                           ? QStringLiteral("用户 #%1").arg(log.operatorId)
                                                           : log.operatorName));
        m_logTable->setItem(i, 5, new QTableWidgetItem(log.remark));
    }
}

void OrderDetailDialog::applyAction(const QString& action)
{
    const int uid = Session::instance().currentUser().id;
    const OpResult r = m_tradeService->changeStatus(m_orderId, action, uid);
    if (!r.ok) {
        QMessageBox::warning(this, QStringLiteral("操作失败"), r.message);
        return;
    }
    accept();   // 父页面收到 Accepted 会 refresh
}
