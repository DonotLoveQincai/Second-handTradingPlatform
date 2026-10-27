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
#include "ui/pages/product/product_ui.h"
#include "ui/pages/product/publish_dialog.h"

MyProductsPage::MyProductsPage(ProductService* service, QWidget* parent)
    : QWidget(parent), m_service(service)
{
    QVBoxLayout* root = new QVBoxLayout(this);

    m_tip = new QLabel(this);
    root->addWidget(m_tip);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"标题", "分类", "价格", "成色", "状态", "操作"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(m_table, 1);
}

void MyProductsPage::refresh()
{
    int uid = Session::instance().currentUser().id;
    if (uid <= 0) {
        m_table->setRowCount(0);
        m_tip->setText("登录之后才能管自己的商品");
        return;
    }
    m_tip->setText("双击一行可以改信息");
    QVector<Product> list = m_service->myProducts(uid);

    m_table->setRowCount(0);
    m_table->setRowCount(list.size());
    for (int i = 0; i < list.size(); i++) {
        Product p = list[i];
        m_table->setItem(i, 0, new QTableWidgetItem(p.title));
        m_table->setItem(i, 1, new QTableWidgetItem(p.categoryName));
        m_table->setItem(i, 2, new QTableWidgetItem(QString("￥%1").arg(p.price, 0, 'f', 2)));
        m_table->setItem(i, 3, new QTableWidgetItem(conditionText(p.conditionLevel)));
        m_table->setItem(i, 4, new QTableWidgetItem(Const::zh(p.status)));

        QWidget* cell = new QWidget(m_table);
        QHBoxLayout* lay = new QHBoxLayout(cell);
        lay->setContentsMargins(2, 2, 2, 2);

        QPushButton* edit = new QPushButton("编辑", cell);
        QPushButton* toggle = new QPushButton(
            p.status == Const::ProductStatus::OffSale ? "上架" : "下架", cell);
        bool sold = (p.status == Const::ProductStatus::Sold);
        edit->setEnabled(!sold);
        toggle->setEnabled(!sold);

        Product copy = p;
        connect(edit, &QPushButton::clicked, this, [this, copy]() { editProduct(copy); });
        if (p.status == Const::ProductStatus::OffSale) {
            int id = p.id;
            connect(toggle, &QPushButton::clicked, this, [this, id]() { putOnSale(id); });
        } else {
            int id = p.id;
            connect(toggle, &QPushButton::clicked, this, [this, id]() { takeDown(id); });
        }

        lay->addWidget(edit);
        lay->addWidget(toggle);
        lay->addStretch();
        m_table->setCellWidget(i, 5, cell);
    }
}

void MyProductsPage::editProduct(const Product& p)
{
    PublishDialog* dlg = new PublishDialog(m_service, Session::instance().currentUser().id, this);
    dlg->setProduct(p);
    dlg->exec();
    delete dlg;
}

void MyProductsPage::takeDown(int productId)
{
    OpResult r = m_service->takeDown(productId, Session::instance().currentUser().id);
    if (!r.ok)
        QMessageBox::warning(this, "失败", r.message);
}

void MyProductsPage::putOnSale(int productId)
{
    OpResult r = m_service->putOnSale(productId, Session::instance().currentUser().id);
    if (!r.ok)
        QMessageBox::warning(this, "失败", r.message);
}
