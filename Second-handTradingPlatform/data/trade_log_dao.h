#pragma once

#include "core/repository/i_trade_log_repository.h"

// 交易日志 DAO：只管 trade_log 表的增和查。
// 用 DbManager::db() 拿全局唯一连接，只写 SQL，不开事务。
class TradeLogDao : public ITradeLogRepository
{
public:
    OpResult insert(TradeLog& log) override;
    QVector<TradeLog> findByOrderId(int orderId) override;
    QVector<TradeLog> search(const TradeLogQuery& q) override;
};
