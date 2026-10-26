#pragma once

#include <QObject>
#include <QVector>

#include "core/common/opresult.h"
#include "core/entity/order.h"

class IOrderRepository;
class IProductRepository;
class ITradeLogRepository;

class OrderService : public QObject
{
    Q_OBJECT
public:
    OrderService(IOrderRepository* orderRepo, IProductRepository* productRepo,
                 ITradeLogRepository* logRepo, QObject* parent = nullptr);

    // 跨三个域的事务：建订单 + 商品置已售 + 写首条日志，任一步失败整体回滚
    OpResult createOrder(int productId, int buyerId, const QString& remark, int* outOrderId);

    QVector<Order> queryOrders(const OrderQuery& q);

signals:
    void orderCreated(int orderId);

private:
    IOrderRepository*    m_orderRepo;
    IProductRepository*  m_productRepo;
    ITradeLogRepository* m_logRepo;
};
