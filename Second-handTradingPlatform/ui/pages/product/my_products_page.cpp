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

MyProductsPage::MyProductsPage(ProductService* service, QWidget* parent)
    : QWidget(parent), m_service(service)
{
    QVBoxLayout* root = new QVBoxLayout(this);

    m_tip = new QLabel(this);
    root->addWidget(m_tip);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"标题", "分类", "价格", "成色", "操作"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(m_table, 1);

    connect(m_table, &QTableWidget::cellDoubleClicked, this, &MyProductsPage::onRowDoubleClicked);

    refresh();
}

void MyProductsPage::refresh()
{
    if (!Session::instance().isLoggedIn()) {
        m_list.clear();
        m_table->setRowCount(0);
        m_tip->setText("登录之后才能管自己的商品");
        return;
    }

    m_tip->setText("双击一行可以改信息");
    m_list = m_service->myProducts(Session::instance().currentUser().id);

    m_table->setRowCount(0);
    m_table->setRowCount(m_list.size());
    for (int i = 0; i < m_list.size(); i++) {
        Product p = m_list[i];
        m_table->setItem(i, 0, new QTableWidgetItem(p.title));
        m_table->setItem(i, 1, new QTableWidgetItem(p.categoryName));
        m_table->setItem(i, 2, new QTableWidgetItem(QString("￥%1").arg(p.price, 0, 'f', 2)));
        m_table->setItem(i, 3, new QTableWidgetItem(conditionText(p.conditionLevel)));

        QWidget* cell = new QWidget(m_table);
        QHBoxLayout* lay = new QHBoxLayout(cell);
        lay->setContentsMargins(2, 2, 2, 2);
        int id = p.id;
        if (p.status == Const::ProductStatus::OnSale) {
            QPushButton* off = new QPushButton("下架", cell);
            connect(off, &QPushButton::clicked, this, [this, id]() { takeDown(id); });
            lay->addWidget(off);
        } else if (p.status == Const::ProductStatus::OffSale) {
            QPushButton* on = new QPushButton("上架", cell);
            connect(on, &QPushButton::clicked, this, [this, id]() { putOnSale(id); });
            lay->addWidget(on);
        }

        if (p.status != Const::ProductStatus::Sold) {
            QPushButton* ed = new QPushButton("修改", cell);
            connect(ed, &QPushButton::clicked, this, [this, id]() { editProduct(id); });
            lay->addWidget(ed);
        } else {
            lay->addWidget(new QLabel("卖掉了", cell));
        }

        lay->addStretch();
        m_table->setCellWidget(i, 4, cell);
    }
}

void MyProductsPage::editProduct(int productId)
{
    std::optional<Product> p = m_service->find(productId);
    if (!p) {
        QMessageBox::warning(this, "提示", "找不到这个商品");
        return;
    }

    PublishDialog dlg(m_service, Session::instance().currentUser().id, this);
    dlg.setProduct(*p);
    dlg.exec();
    refresh();
}

void MyProductsPage::takeDown(int productId)
{
    OpResult r = m_service->takeDown(productId, Session::instance().currentUser().id);
    if (!r.ok)
        QMessageBox::warning(this, "下架失败", r.message);
    refresh();
}

void MyProductsPage::putOnSale(int productId)
{
    OpResult r = m_service->putOnSale(productId, Session::instance().currentUser().id);
    if (!r.ok)
        QMessageBox::warning(this, "上架失败", r.message);
    refresh();
}

void MyProductsPage::onRowDoubleClicked(int row, int column)
{
    Q_UNUSED(column)
    if (row < 0 || row >= m_list.size())
        return;
    if (m_list[row].status == Const::ProductStatus::Sold)
        return;
    editProduct(m_list[row].id);
}
