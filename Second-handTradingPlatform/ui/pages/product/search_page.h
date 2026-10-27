#pragma once
#include <QWidget>
#include <QVector>

#include "core/entity/product.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QTableWidget;
class ProductService;

// 商品检索页 [A]：默认页。左边分类列表，右边条件 + 表格。
// 双击一行只 emit buyRequested(int)，不知道购买框长什么样（页面之间零耦合）。
class SearchPage : public QWidget
{
    Q_OBJECT
public:
    explicit SearchPage(ProductService* service, QWidget* parent = nullptr);

    void refresh();

signals:
    void buyRequested(int productId);
    void publishRequested();

private slots:
    void onSearchClicked();
    void onCategoryChanged();
    void onRowDoubleClicked(int row, int column);

private:
    void buildUi();
    void loadCategories();
    void fillTable(const QVector<Product>& products);
    QVector<int> selectedCategoryIds() const;

    ProductService* m_service = nullptr;

    QListWidget* m_categoryList = nullptr;
    QLineEdit* m_keywordEdit = nullptr;
    QDoubleSpinBox* m_priceMin = nullptr;
    QDoubleSpinBox* m_priceMax = nullptr;
    QComboBox* m_sortBox = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_countLabel = nullptr;

    QVector<CategoryNode> m_categories;
    QVector<Product> m_rows;          // 表格第 N 行 → 商品实体
};
