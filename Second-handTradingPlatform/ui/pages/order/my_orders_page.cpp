#include "ui/pages/order/my_orders_page.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "core/common/constants.h"
#include "core/entity/order.h"
#include "services/order_service.h"
#include "ui/pages/order/order_detail_dialog.h"

MyOrdersPage::MyOrdersPage(OrderService* orderService, TradeService* tradeService,
                           int userId, bool asSeller, QWidget* parent)
    : QWidget(parent)
    , m_orderService(orderService)
    , m_tradeService(tradeService)
    , m_userId(userId)
    , m_asSeller(asSeller)
{
    m_statusBox = new QComboBox(this);
    m_statusBox->addItem(QStringLiteral("全部"), QString());
    m_statusBox->addItem(Const::zh(Const::OrderStatus::PendingPay), Const::OrderStatus::PendingPay);
    m_statusBox->addItem(Const::zh(Const::OrderStatus::Paid),       Const::OrderStatus::Paid);
    m_statusBox->addItem(Const::zh(Const::OrderStatus::Shipped),    Const::OrderStatus::Shipped);
    m_statusBox->addItem(Const::zh(Const::OrderStatus::Completed),  Const::OrderStatus::Completed);
    m_statusBox->addItem(Const::zh(Const::OrderStatus::Cancelled),  Const::OrderStatus::Cancelled);

    m_keywordEdit = new QLineEdit(this);
    m_keywordEdit->setPlaceholderText(QStringLiteral("订单号或商品名"));

    auto* queryBtn = new QPushButton(QStringLiteral("查询"), this);

    auto* bar = new QHBoxLayout;
    bar->addWidget(m_statusBox);
    bar->addWidget(m_keywordEdit);
    bar->addWidget(queryBtn);
    bar->addStretch();

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({m_asSeller ? QStringLiteral("买家") : QStringLiteral("卖家"),
                                        QStringLiteral("商品"),
                                        QStringLiteral("金额"),
                                        QStringLiteral("状态"),
                                        QStringLiteral("下单时间")});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(bar);
    layout->addWidget(m_table);

    connect(queryBtn, &QPushButton::clicked, this, &MyOrdersPage::refresh);
    connect(m_statusBox, &QComboBox::currentIndexChanged, this, [this](int) { refresh(); });
    connect(m_keywordEdit, &QLineEdit::returnPressed, this, &MyOrdersPage::refresh);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) { openDetail(row); });

    refresh();
}

void MyOrdersPage::refresh()
{
    OrderQuery q;
    if (m_asSeller)
        q.sellerId = m_userId;
    else
        q.buyerId = m_userId;
    q.status  = m_statusBox->currentData().toString();
    q.keyword = m_keywordEdit->text().trimmed();

    const QVector<Order> list = m_orderService->queryOrders(q);

    m_table->setRowCount(list.size());
    for (int i = 0; i < list.size(); ++i) {
        const Order& o = list.at(i);

        auto* first = new QTableWidgetItem(m_asSeller ? o.buyerName : o.sellerName);
        first->setData(Qt::UserRole, o.id);
        m_table->setItem(i, 0, first);

        m_table->setItem(i, 1, new QTableWidgetItem(
            o.productTitle.isEmpty() ? QStringLiteral("商品 #%1").arg(o.productId) : o.productTitle));
        m_table->setItem(i, 2, new QTableWidgetItem(QStringLiteral("￥%1").arg(o.amount, 0, 'f', 2)));
        m_table->setItem(i, 3, new QTableWidgetItem(Const::zh(o.status)));
        m_table->setItem(i, 4, new QTableWidgetItem(o.createdAt));
    }
}

void MyOrdersPage::setUserId(int userId)
{
    m_userId = userId;
    refresh();
}

void MyOrdersPage::openDetail(int row)
{
    const QTableWidgetItem* first = m_table->item(row, 0);
    if (!first)
        return;

    OrderDetailDialog dlg(m_orderService, m_tradeService, first->data(Qt::UserRole).toInt(), this);
    if (dlg.exec() == QDialog::Accepted)
        refresh();   // 详情页里改过状态，回来要重查
}
