#pragma once

#include "core/repository/i_order_repository.h"

class OrderDao : public IOrderRepository
{
public:
    OpResult insert(Order& o) override;
    OpResult updateStatus(int orderId, const QString& from, const QString& to) override;
    std::optional<Order> findById(int id) override;
    QVector<Order> search(const OrderQuery& cond) override;
    QString nextOrderNo() override;
};
