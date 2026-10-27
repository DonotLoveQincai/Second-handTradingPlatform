#include "ui/pages/order/buy_dialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "core/common/opresult.h"
#include "services/order_service.h"
#include "services/product_service.h"
#include "ui/pages/product/product_ui.h"

BuyDialog::BuyDialog(OrderService* orderService, ProductService* productService,
                     int productId, int buyerId, QWidget* parent)
    : QDialog(parent)
    , m_orderService(orderService)
    , m_productService(productService)
    , m_productId(productId)
    , m_buyerId(buyerId)
{
    setWindowTitle(QStringLiteral("确认购买"));
    setMinimumWidth(360);

    const auto p = m_productService->find(m_productId);
    if (!p) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("商品不存在或已被删除"));
        reject();
        return;
    }

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("商品"), new QLabel(p->title, this));
    form->addRow(QStringLiteral("分类"),
                 new QLabel(p->categoryName.isEmpty() ? QStringLiteral("未分类") : p->categoryName, this));
    form->addRow(QStringLiteral("价格"), new QLabel(QStringLiteral("￥%1").arg(p->price, 0, 'f', 2), this));
    form->addRow(QStringLiteral("卖家"),
                 new QLabel(p->sellerName.isEmpty() ? QStringLiteral("卖家 #%1").arg(p->sellerId) : p->sellerName, this));
    form->addRow(QStringLiteral("成色"), new QLabel(conditionText(p->conditionLevel), this));

    m_remarkEdit = new QLineEdit(this);
    m_remarkEdit->setPlaceholderText(QStringLiteral("选填，比如：周末面交"));
    form->addRow(QStringLiteral("备注"), m_remarkEdit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确认购买"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons, &QDialogButtonBox::accepted, this, &BuyDialog::confirm);
    connect(buttons, &QDialogButtonBox::rejected, this, &BuyDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void BuyDialog::confirm()
{
    const OpResult r = m_orderService->createOrder(m_productId, m_buyerId,
                                                   m_remarkEdit->text().trimmed(), nullptr);
    if (!r.ok) {
        // 商品被别人抢走、或状态已变，都在这里；窗口不关，用户可以退出去刷新
        QMessageBox::warning(this, QStringLiteral("下单失败"), r.message);
        return;
    }

    QMessageBox::information(this, QStringLiteral("下单成功"),
                             QStringLiteral("订单已创建，请到「我的订单」查看"));
    accept();
}
