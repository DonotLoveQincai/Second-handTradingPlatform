#include "services/trade_service.h"

#include <QSqlDatabase>

#include "core/common/constants.h"
#include "data/db_manager.h"
#include "services/trade_state_machine.h"

TradeService::TradeService(IOrderRepository* orderRepo,
                           IProductRepository* productRepo,
                           ITradeLogRepository* logRepo,
                           QObject* parent)
    : QObject(parent)
    , m_orderRepo(orderRepo)
    , m_productRepo(productRepo)
    , m_logRepo(logRepo)
{
}

OpResult TradeService::changeStatus(int orderId, const QString& action, int operatorId)
{
    auto order = m_orderRepo->findById(orderId);
    if (!order)
        return OpResult::fail("订单不存在");

    // ① 状态机校验（纯逻辑，不问数据库）
    const QString from = order->status;
    if (!TradeStateMachine::canTransit(from, action))
        return OpResult::fail("当前状态不能执行该操作");
    const QString to = TradeStateMachine::nextStatus(from, action);

    // ② 权限校验（按买卖双方身份推导）
    OpResult r = checkPermission(*order, action, operatorId);
    if (!r.ok)
        return r;

    // ③ 一个事务里：改订单 + 写日志 +（取消时）把商品放回在售
    QSqlDatabase& db = DbManager::db();
    if (!db.transaction())
        return OpResult::fail("开启事务失败");

    // 条件更新：只有订单还是刚读到的旧状态才改；影响行数 0 = 被别人抢先改了，回滚。
    r = m_orderRepo->updateStatus(orderId, from, to);
    if (!r.ok) {
        db.rollback();
        return OpResult::fail("订单状态已变，请刷新后重试");
    }

    TradeLog log;
    log.orderId    = orderId;
    log.fromStatus = from;
    log.toStatus   = to;
    log.action     = action;
    log.operatorId = operatorId;
    log.remark     = TradeStateMachine::actionName(action);
    r = m_logRepo->insert(log);
    if (!r.ok) {
        db.rollback();
        return r;
    }

    // 取消订单：商品从"已售出"放回"在售"（不做这一步，订单没了商品也永远卖不掉）
    if (action == Const::TradeAction::Cancel) {
        r = m_productRepo->updateStatus(order->productId,
                                        Const::ProductStatus::Sold,
                                        Const::ProductStatus::OnSale);
        if (!r.ok) {
            db.rollback();
            return OpResult::fail("释放商品失败，取消失败");
        }
    }

    if (!db.commit()) {
        db.rollback();
        return OpResult::fail("提交事务失败");
    }

    emit orderStatusChanged(orderId, to);
    return OpResult::success();
}

OpResult TradeService::checkPermission(const Order& o, const QString& action, int uid)
{
    const bool isBuyer  = (uid == o.buyerId);
    const bool isSeller = (uid == o.sellerId);

    if (action == Const::TradeAction::Pay || action == Const::TradeAction::Confirm)
        return isBuyer ? OpResult::success() : OpResult::fail("只有买家可以执行此操作");

    if (action == Const::TradeAction::Ship)
        return isSeller ? OpResult::success() : OpResult::fail("只有卖家可以执行此操作");

    if (action == Const::TradeAction::Cancel) {
        if (o.status == Const::OrderStatus::PendingPay)
            return (isBuyer || isSeller) ? OpResult::success()
                                         : OpResult::fail("无权取消该订单");
        return isSeller ? OpResult::success() : OpResult::fail("已付款的订单只有卖家可以取消");
    }
    return OpResult::fail("未知操作");
}

QVector<TradeLog> TradeService::orderLogs(int orderId)
{
    TradeLogQuery q;
    q.orderId = orderId;
    return m_logRepo->search(q);   // 订单详情页用，只看这一单
}

QVector<TradeLog> TradeService::queryLogs(const TradeLogQuery& q)
{
    return m_logRepo->search(q);   // 交易记录页用，可按关键字/状态筛
}
