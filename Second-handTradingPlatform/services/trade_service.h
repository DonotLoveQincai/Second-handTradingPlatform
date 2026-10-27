#pragma once

#include <QObject>
#include <QVector>

#include "core/common/opresult.h"
#include "core/entity/order.h"
#include "core/entity/trade_log.h"
#include "core/repository/i_order_repository.h"
#include "core/repository/i_product_repository.h"
#include "core/repository/i_trade_log_repository.h"

// 交易服务：状态流转（状态机校验 + 权限校验 + 一个事务改三张表）+ 日志查询。
// 依赖 B 的订单仓储、A 的商品仓储、自己的日志仓储，全部构造时注入，自己不 new。
class TradeService : public QObject
{
    Q_OBJECT

public:
    TradeService(IOrderRepository* orderRepo,
                 IProductRepository* productRepo,
                 ITradeLogRepository* logRepo,
                 QObject* parent = nullptr);

    // 对订单执行一个动作（付款/发货/确认收货/取消）。状态机不允许或权限不合都返回失败。
    OpResult changeStatus(int orderId, const QString& action, int operatorId);

    // 某一单的全部日志（订单详情页用），按 id 正序
    QVector<TradeLog> orderLogs(int orderId);

    // 全平台日志查询（交易记录页用），条件可空
    QVector<TradeLog> queryLogs(const TradeLogQuery& q);

signals:
    void orderStatusChanged(int orderId, const QString& newStatus);

private:
    // 按订单归属推导权限：库里没有"角色"字段，归属才靠谱
    OpResult checkPermission(const Order& o, const QString& action, int uid);

    IOrderRepository*    m_orderRepo   = nullptr;
    IProductRepository*  m_productRepo = nullptr;
    ITradeLogRepository* m_logRepo     = nullptr;
};
