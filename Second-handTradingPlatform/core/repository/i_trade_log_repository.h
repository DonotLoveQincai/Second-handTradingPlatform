#pragma once

#include <QVector>

#include "core/common/opresult.h"
#include "core/entity/trade_log.h"

// 交易日志仓储接口：签名定死，B 下单、C 改状态都靠它写日志。
// DAO 一律不开事务，事务由调用者（createOrder / changeStatus）开。
class ITradeLogRepository
{
public:
    virtual ~ITradeLogRepository() = default;

    virtual OpResult insert(TradeLog& log) = 0;
    virtual QVector<TradeLog> findByOrderId(int orderId) = 0;   // 按 id 正序
    virtual QVector<TradeLog> search(const TradeLogQuery& q) = 0;
};
