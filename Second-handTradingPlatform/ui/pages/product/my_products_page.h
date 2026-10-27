#pragma once
#include <QWidget>
#include <QVector>

#include "core/entity/product.h"

class QLabel;
class QTableWidget;
class ProductService;

// 我的商品页 [A]：只能看到自己的商品；操作按钮放在表格最后一列，按状态决定显示哪几个：
//   ON_SALE  → 下架 + 修改
//   OFF_SALE → 上架 + 修改
//   SOLD     → 一个都不显示（已成交的东西不许改、不许动）
class MyProductsPage : public QWidget
{
    Q_OBJECT
public:
    explicit MyProductsPage(ProductService* service, QWidget* parent = nullptr);

    void refresh();

private slots:
    void onRowDoubleClicked(int row, int column);

private:
    void buildUi();
    void editProduct(int productId);
    void takeDown(int productId);
    void putOnSale(int productId);

    ProductService* m_service = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_hintLabel = nullptr;
    QVector<Product> m_rows;
};
