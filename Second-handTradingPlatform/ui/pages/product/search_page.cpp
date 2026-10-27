#include "ui/pages/product/search_page.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "core/session.h"
#include "services/product_service.h"

SearchPage::SearchPage(ProductService* service, QWidget* parent)
    : QWidget(parent), m_service(service)
{
    QHBoxLayout* root = new QHBoxLayout(this);

    QVBoxLayout* left = new QVBoxLayout;
    left->addWidget(new QLabel("分类", this));
    m_catList = new QListWidget(this);
    m_catList->setFixedWidth(150);
    left->addWidget(m_catList, 1);
    root->addLayout(left);

    QVBoxLayout* right = new QVBoxLayout;

    QHBoxLayout* row1 = new QHBoxLayout;
    row1->addWidget(new QLabel("关键字", this));
    m_kw = new QLineEdit(this);
    m_kw->setPlaceholderText("标题里有什么");
    m_kw->setClearButtonEnabled(true);
    row1->addWidget(m_kw, 1);

    row1->addWidget(new QLabel("价格", this));
    m_min = new QDoubleSpinBox(this);
    m_min->setRange(0, 1000000);
    m_min->setDecimals(0);
    m_min->setSpecialValueText("不限");
    row1->addWidget(m_min);
    row1->addWidget(new QLabel("-", this));
    m_max = new QDoubleSpinBox(this);
    m_max->setRange(0, 1000000);
    m_max->setDecimals(0);
    m_max->setSpecialValueText("不限");
    row1->addWidget(m_max);

    row1->addWidget(new QLabel("排序", this));
    m_sort = new QComboBox(this);
    m_sort->addItem("最新发布", "time_desc");
    m_sort->addItem("价格从低到高", "price_asc");
    m_sort->addItem("价格从高到低", "price_desc");
    row1->addWidget(m_sort);

    QPushButton* searchBtn = new QPushButton("搜索", this);
    QPushButton* pubBtn = new QPushButton("发布商品", this);
    row1->addWidget(searchBtn);
    row1->addWidget(pubBtn);
    right->addLayout(row1);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"标题", "分类", "价格", "成色", "卖家", "发布时间"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    right->addWidget(m_table, 1);

    m_count = new QLabel(this);
    right->addWidget(m_count);
    root->addLayout(right, 1);

    m_catList->addItem("全部");
    m_catList->item(0)->setData(Qt::UserRole, 0);
    m_cats = m_service->categories();
    for (int i = 0; i < m_cats.size(); i++) {
        if (m_cats[i].parentId != 0)
            continue;
        m_catList->addItem(m_cats[i].name);
        m_catList->item(m_catList->count() - 1)->setData(Qt::UserRole, m_cats[i].id);
        for (int j = 0; j < m_cats.size(); j++) {
            if (m_cats[j].parentId == m_cats[i].id) {
                m_catList->addItem(QString("    ") + m_cats[j].name);
                m_catList->item(m_catList->count() - 1)->setData(Qt::UserRole, m_cats[j].id);
            }
        }
    }

    connect(searchBtn, &QPushButton::clicked, this, &SearchPage::refresh);
    connect(pubBtn, &QPushButton::clicked, this, &SearchPage::publishRequested);
    connect(m_kw, &QLineEdit::returnPressed, this, &SearchPage::refresh);
    connect(m_sort, &QComboBox::currentIndexChanged, this, &SearchPage::refresh);
    connect(m_catList, &QListWidget::currentRowChanged, this, &SearchPage::refresh);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &SearchPage::onRowDoubleClicked);

    m_catList->setCurrentRow(0);
    refresh();
}

QVector<int> SearchPage::selectedIds() const
{
    QVector<int> ids;
    QListWidgetItem* item = m_catList->currentItem();
    if (!item)
        return ids;
    int cid = item->data(Qt::UserRole).toInt();
    if (cid <= 0)
        return ids;
    ids.append(cid);
    for (int i = 0; i < m_cats.size(); i++) {
        if (m_cats[i].parentId == cid)
            ids.append(m_cats[i].id);
    }
    return ids;
}

void SearchPage::refresh()
{
    ProductQuery q;
    q.keyword = m_kw->text().trimmed();
    q.categoryIds = selectedIds();
    if (m_min->value() > 0)
        q.priceMin = m_min->value();
    if (m_max->value() > 0)
        q.priceMax = m_max->value();
    q.sortBy = m_sort->currentData().toString();

    if (q.priceMin >= 0 && q.priceMax >= 0 && q.priceMin > q.priceMax) {
        QMessageBox::warning(this, "价格区间", "最低价比最高价还高");
        return;
    }

    m_list = m_service->search(q);

    m_table->setRowCount(0);
    m_table->setRowCount(m_list.size());
    for (int i = 0; i < m_list.size(); i++) {
        Product p = m_list[i];
        m_table->setItem(i, 0, new QTableWidgetItem(p.title));
        m_table->setItem(i, 1, new QTableWidgetItem(p.categoryName));
        m_table->setItem(i, 2, new QTableWidgetItem(QString("￥%1").arg(p.price, 0, 'f', 2)));
        m_table->setItem(i, 3, new QTableWidgetItem(conditionText(p.conditionLevel)));
        m_table->setItem(i, 4, new QTableWidgetItem(p.sellerName));
        m_table->setItem(i, 5, new QTableWidgetItem(p.createdAt));
    }
    m_count->setText(QString("一共 %1 件，双击可以买").arg(m_table->rowCount()));
}

void SearchPage::onRowDoubleClicked(int row, int column)
{
    Q_UNUSED(column)
    if (row < 0 || row >= m_list.size())
        return;
    Product p = m_list[row];
    if (Session::instance().isLoggedIn() && p.sellerId == Session::instance().currentUser().id) {
        QMessageBox::information(this, "提示", "这是你自己发布的商品");
        return;
    }
    emit buyRequested(p.id);
}
