#pragma once
#include <QWidget>
#include <QVector>

#include "core/entity/product.h"

class QLabel;
class QTableWidget;
class ProductService;

class MyProductsPage : public QWidget
{
    Q_OBJECT
public:
    explicit MyProductsPage(ProductService* service, QWidget* parent = nullptr);

    void refresh();

private:
    void editProduct(const Product& p);
    void takeDown(int productId);
    void putOnSale(int productId);

    ProductService* m_service = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_tip = nullptr;
};
