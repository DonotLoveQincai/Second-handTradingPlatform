#pragma once
#include <QWidget>
#include <QVector>

#include "core/entity/product.h"
#include "ui/pages/product/product_ui.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QTableWidget;
class ProductService;

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
    void onRowDoubleClicked(int row, int column);

private:
    QVector<int> selectedIds() const;

    ProductService* m_service = nullptr;
    QListWidget* m_catList = nullptr;
    QLineEdit* m_kw = nullptr;
    QDoubleSpinBox* m_min = nullptr;
    QDoubleSpinBox* m_max = nullptr;
    QComboBox* m_sort = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_count = nullptr;
    QVector<CategoryItem> m_cats;
    QVector<Product> m_list;
};
