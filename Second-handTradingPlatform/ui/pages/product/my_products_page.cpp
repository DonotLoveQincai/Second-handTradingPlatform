#include "ui/pages/product/my_products_page.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "core/common/constants.h"
#include "core/session.h"
#include "services/product_service.h"
#include "ui/pages/product/publish_dialog.h"

namespace {
constexpr int kColTitle = 0;
constexpr int kColCategory = 1;
constexpr int kColPrice = 2;
constexpr int kColCondition = 3;
constexpr int kColStatus = 4;
constexpr int kColActions = 5;
constexpr int kColCount = 6;
}

MyProductsPage::MyProductsPage(ProductService* service, QWidget* parent)
    : QWidget(parent)
    , m_service(service)
{
    buildUi();
    refresh();
}

void MyProductsPage::buildUi()
{
    auto* root = new QVBoxLayout(this);

    m_hintLabel = new QLabel(this);
    root->addWidget(m_hintLabel);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(kColCount);
    m_table->setHorizontalHeaderLabels({QStringLiteral("标题"), QStringLiteral("分类"),
                                        QStringLiteral("价格"), QStringLiteral("成色"),
                                        QStringLiteral("状态"), QStringLiteral("操作")});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(m_table, 1);

    connect(m_table, &QTableWidget::cellDoubleClicked, this, &MyProductsPage::onRowDoubleClicked);
}

void MyProductsPage::refresh()
{
    if (!Session::instance().isLoggedIn()) {
        m_rows.clear();
        m_table->setRowCount(0);
        m_hintLabel->setText(QStringLiteral("登录后可以管理自己发布的商品。"));
        return;
    }

    m_hintLabel->setText(QStringLiteral("点按钮上架 / 下架 / 改信息；双击一行也能改信息。"));

    m_rows = m_service->myProducts(Session::instance().currentUser().id);

    // 每行都 new 了 item 和按钮控件，先清空再重建（否则越刷越多）
    m_table->setRowCount(0);
    m_table->setRowCount(m_rows.size());

    for (int row = 0; row < m_rows.size(); ++row) {
        const Product& product = m_rows.at(row);

        m_table->setItem(row, kColTitle, new QTableWidgetItem(product.title));
        m_table->setItem(row, kColCategory, new QTableWidgetItem(product.categoryName));
        m_table->setItem(row, kColPrice,
                         new QTableWidgetItem(QStringLiteral("￥%1").arg(product.price, 0, 'f', 2)));
        m_table->setItem(row, kColCondition, new QTableWidgetItem(conditionText(product.conditionLevel)));
        m_table->setItem(row, kColStatus, new QTableWidgetItem(Const::zh(product.status)));

        auto* cell = new QWidget(m_table);
        auto* cellLayout = new QHBoxLayout(cell);
        cellLayout->setContentsMargins(2, 2, 2, 2);
        cellLayout->setSpacing(4);

        const int productId = product.id;       // 按值捕获，否则循环里全拿到最后一个 id
        if (product.status == Const::ProductStatus::OnSale) {
            auto* offButton = new QPushButton(QStringLiteral("下架"), cell);
            connect(offButton, &QPushButton::clicked, this,
                    [this, productId]() { takeDown(productId); });
            cellLayout->addWidget(offButton);
        } else if (product.status == Const::ProductStatus::OffSale) {
            auto* onButton = new QPushButton(QStringLiteral("上架"), cell);
            connect(onButton, &QPushButton::clicked, this,
                    [this, productId]() { putOnSale(productId); });
            cellLayout->addWidget(onButton);
        }

        if (product.status != Const::ProductStatus::Sold) {
            auto* editButton = new QPushButton(QStringLiteral("修改"), cell);
            connect(editButton, &QPushButton::clicked, this,
                    [this, productId]() { editProduct(productId); });
            cellLayout->addWidget(editButton);
        } else {
            cellLayout->addWidget(new QLabel(QStringLiteral("已成交，不可操作"), cell));
        }

        cellLayout->addStretch();
        m_table->setCellWidget(row, kColActions, cell);
    }
}

void MyProductsPage::editProduct(int productId)
{
    const std::optional<Product> product = m_service->find(productId);
    if (!product) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("商品不存在"));
        return;
    }

    PublishDialog dialog(m_service, Session::instance().currentUser().id, this);
    dialog.setProduct(*product);
    dialog.exec();
    refresh();
}

void MyProductsPage::takeDown(int productId)
{
    const OpResult result = m_service->takeDown(productId, Session::instance().currentUser().id);
    if (!result.ok)
        QMessageBox::warning(this, QStringLiteral("下架失败"), result.message);
    refresh();
}

void MyProductsPage::putOnSale(int productId)
{
    const OpResult result = m_service->putOnSale(productId, Session::instance().currentUser().id);
    if (!result.ok)
        QMessageBox::warning(this, QStringLiteral("上架失败"), result.message);
    refresh();
}

void MyProductsPage::onRowDoubleClicked(int row, int column)
{
    Q_UNUSED(column)
    if (row < 0 || row >= m_rows.size())
        return;
    if (m_rows.at(row).status == Const::ProductStatus::Sold)
        return;
    editProduct(m_rows.at(row).id);
}
