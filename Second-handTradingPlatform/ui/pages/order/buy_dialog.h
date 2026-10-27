#pragma once
#include <QDialog>

class OrderService;
class ProductService;
class QLineEdit;

// 购买确认框：显示商品信息 + 备注，确认后走 OrderService::createOrder。
class BuyDialog : public QDialog
{
    Q_OBJECT

public:
    BuyDialog(OrderService* orderService, ProductService* productService,
              int productId, int buyerId, QWidget* parent = nullptr);

private:
    void confirm();

    OrderService*   m_orderService   = nullptr;
    ProductService* m_productService = nullptr;
    int             m_productId = 0;
    int             m_buyerId   = 0;
    QLineEdit*      m_remarkEdit = nullptr;
};
