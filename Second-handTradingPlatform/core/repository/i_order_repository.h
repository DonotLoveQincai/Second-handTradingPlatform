#pragma once

#include <optional>

#include <QString>
#include <QVector>

#include "core/common/opresult.h"
#include "core/entity/order.h"

class IOrderRepository
{
public:
    virtual ~IOrderRepository() = default;

    // 成功后回填 o.id
    virtual OpResult insert(Order& o) = 0;

    // 当前状态等于 from 才改成 to，没改到返回失败
    virtual OpResult updateStatus(int orderId, const QString& from, const QString& to) = 0;

    virtual std::optional<Order> findById(int id) = 0;
    virtual QVector<Order> search(const OrderQuery& q) = 0;

    // 当天最大单号 +1，形如 20261025-0001
    virtual QString nextOrderNo() = 0;
};
