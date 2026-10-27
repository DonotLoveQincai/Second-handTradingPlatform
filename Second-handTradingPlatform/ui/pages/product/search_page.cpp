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

#include "core/common/constants.h"
#include "core/session.h"
#include "services/product_service.h"

namespace {
constexpr int kColTitle = 0;
constexpr int kColCategory = 1;
constexpr int kColPrice = 2;
constexpr int kColCondition = 3;
constexpr int kColSeller = 4;
constexpr int kColStatus = 5;
constexpr int kColCreated = 6;
constexpr int kColCount = 7;

QString moneyText(double value)
{
    return QStringLiteral("￥%1").arg(value, 0, 'f', 2);
}
}

SearchPage::SearchPage(ProductService* service, QWidget* parent)
    : QWidget(parent)
    , m_service(service)
{
    buildUi();
    loadCategories();
    refresh();
}

void SearchPage::buildUi()
{
    auto* root = new QHBoxLayout(this);

    // —— 左侧：分类列表（全部 + 父分类 + 子分类） ——
    auto* leftColumn = new QVBoxLayout;
    leftColumn->addWidget(new QLabel(QStringLiteral("分类"), this));
    m_categoryList = new QListWidget(this);
    m_categoryList->setFixedWidth(150);
    leftColumn->addWidget(m_categoryList, 1);
    root->addLayout(leftColumn);

    // —— 右侧：条件 + 表格 ——
    auto* rightColumn = new QVBoxLayout;

    auto* filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel(QStringLiteral("关键字"), this));
    m_keywordEdit = new QLineEdit(this);
    m_keywordEdit->setPlaceholderText(QStringLiteral("标题包含…"));
    m_keywordEdit->setClearButtonEnabled(true);
    filterRow->addWidget(m_keywordEdit, 1);

    filterRow->addWidget(new QLabel(QStringLiteral("价格"), this));
    m_priceMin = new QDoubleSpinBox(this);
    m_priceMin->setRange(0, 1000000);
    m_priceMin->setDecimals(0);
    m_priceMin->setSpecialValueText(QStringLiteral("不限"));
    filterRow->addWidget(m_priceMin);

    filterRow->addWidget(new QLabel(QStringLiteral("-"), this));
    m_priceMax = new QDoubleSpinBox(this);
    m_priceMax->setRange(0, 1000000);
    m_priceMax->setDecimals(0);
    m_priceMax->setSpecialValueText(QStringLiteral("不限"));
    filterRow->addWidget(m_priceMax);

    filterRow->addWidget(new QLabel(QStringLiteral("排序"), this));
    m_sortBox = new QComboBox(this);
    m_sortBox->addItem(QStringLiteral("最新发布"), QStringLiteral("time_desc"));
    m_sortBox->addItem(QStringLiteral("价格从低到高"), QStringLiteral("price_asc"));
    m_sortBox->addItem(QStringLiteral("价格从高到低"), QStringLiteral("price_desc"));
    filterRow->addWidget(m_sortBox);

    auto* searchButton = new QPushButton(QStringLiteral("搜索"), this);
    auto* publishButton = new QPushButton(QStringLiteral("发布商品"), this);
    filterRow->addWidget(searchButton);
    filterRow->addWidget(publishButton);
    rightColumn->addLayout(filterRow);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(kColCount);
    m_table->setHorizontalHeaderLabels({QStringLiteral("标题"), QStringLiteral("分类"),
                                        QStringLiteral("价格"), QStringLiteral("成色"),
                                        QStringLiteral("卖家"), QStringLiteral("状态"),
                                        QStringLiteral("发布时间")});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    rightColumn->addWidget(m_table, 1);

    m_countLabel = new QLabel(this);
    rightColumn->addWidget(m_countLabel);

    root->addLayout(rightColumn, 1);

    connect(searchButton, &QPushButton::clicked, this, &SearchPage::onSearchClicked);
    connect(publishButton, &QPushButton::clicked, this, &SearchPage::publishRequested);
    connect(m_keywordEdit, &QLineEdit::returnPressed, this, &SearchPage::onSearchClicked);
    connect(m_sortBox, &QComboBox::currentIndexChanged, this, &SearchPage::onSearchClicked);
    connect(m_categoryList, &QListWidget::currentRowChanged, this, &SearchPage::onCategoryChanged);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &SearchPage::onRowDoubleClicked);
}

// 分类列：全部 → 父分类 → 缩进的子分类；id 存在 Qt::UserRole 里
void SearchPage::loadCategories()
{
    m_categories = m_service->categories();

    m_categoryList->clear();
    auto* allItem = new QListWidgetItem(QStringLiteral("全部"), m_categoryList);
    allItem->setData(Qt::UserRole, 0);

    for (const CategoryNode& parent : m_categories) {
        if (parent.parentId != 0)
            continue;
        auto* parentItem = new QListWidgetItem(parent.name, m_categoryList);
        parentItem->setData(Qt::UserRole, parent.id);
        for (const CategoryNode& child : m_categories) {
            if (child.parentId != parent.id)
                continue;
            auto* childItem = new QListWidgetItem(QStringLiteral("    ") + child.name, m_categoryList);
            childItem->setData(Qt::UserRole, child.id);
        }
    }

    if (m_categoryList->count() > 0)
        m_categoryList->setCurrentRow(0);
}

void SearchPage::refresh()
{
    ProductQuery query;
    query.keyword = m_keywordEdit->text().trimmed();
    query.categoryIds = selectedCategoryIds();
    if (m_priceMin->value() > 0)
        query.priceMin = m_priceMin->value();
    if (m_priceMax->value() > 0)
        query.priceMax = m_priceMax->value();
    query.sortBy = m_sortBox->currentData().toString();

    if (query.priceMin >= 0 && query.priceMax >= 0 && query.priceMin > query.priceMax) {
        QMessageBox::warning(this, QStringLiteral("价格区间"), QStringLiteral("最低价不能大于最高价"));
        return;
    }

    fillTable(m_service->search(query));
}

// 选中父分类时子分类一起查，否则「数码电子」查不到「手机」下的商品
QVector<int> SearchPage::selectedCategoryIds() const
{
    QVector<int> ids;
    if (!m_categoryList->currentItem())
        return ids;

    const int selectedId = m_categoryList->currentItem()->data(Qt::UserRole).toInt();
    if (selectedId <= 0)
        return ids;

    ids.append(selectedId);
    for (const CategoryNode& node : m_categories) {
        if (node.parentId == selectedId)
            ids.append(node.id);
    }
    return ids;
}

void SearchPage::fillTable(const QVector<Product>& products)
{
    m_rows = products;

    // 每行都 new QTableWidgetItem，刷新前必须先清空，否则越刷越多
    m_table->setRowCount(0);
    m_table->setRowCount(products.size());
    for (int row = 0; row < products.size(); ++row) {
        const Product& p = products.at(row);
        m_table->setItem(row, kColTitle, new QTableWidgetItem(p.title));
        m_table->setItem(row, kColCategory, new QTableWidgetItem(p.categoryName));
        m_table->setItem(row, kColPrice, new QTableWidgetItem(moneyText(p.price)));
        m_table->setItem(row, kColCondition, new QTableWidgetItem(conditionText(p.conditionLevel)));
        m_table->setItem(row, kColSeller, new QTableWidgetItem(p.sellerName));
        m_table->setItem(row, kColStatus, new QTableWidgetItem(Const::zh(p.status)));
        m_table->setItem(row, kColCreated, new QTableWidgetItem(p.createdAt));
    }

    m_countLabel->setText(QStringLiteral("共 %1 件商品（双击一行下单）").arg(m_table->rowCount()));
}

void SearchPage::onSearchClicked()
{
    refresh();
}

void SearchPage::onCategoryChanged()
{
    refresh();
}

void SearchPage::onRowDoubleClicked(int row, int column)
{
    Q_UNUSED(column)
    if (row < 0 || row >= m_rows.size())
        return;

    const Product& product = m_rows.at(row);

    // 自己的商品不给买，先在界面上拦一次（OrderService 里还会再拦一次）
    if (Session::instance().isLoggedIn()
        && product.sellerId == Session::instance().currentUser().id) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("这是你自己发布的商品"));
        return;
    }

    if (product.status != Const::ProductStatus::OnSale) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("该商品当前是「%1」，不能下单").arg(Const::zh(product.status)));
        return;
    }

    emit buyRequested(product.id);
}
