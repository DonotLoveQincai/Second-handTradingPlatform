#include "services/order_service.h"

#include <QSqlDatabase>

#include "core/common/constants.h"
#include "core/entity/product.h"
#include "core/entity/trade_log.h"
#include "core/repository/i_order_repository.h"
#include "core/repository/i_product_repository.h"
#include "core/repository/i_trade_log_repository.h"
#include "data/db_manager.h"

OrderService::OrderService(IOrderRepository* orderRepo, IProductRepository* productRepo,
                           ITradeLogRepository* logRepo, QObject* parent)
    : QObject(parent)
    , m_orderRepo(orderRepo)
    , m_productRepo(productRepo)
    , m_logRepo(logRepo)
{
}

OpResult OrderService::createOrder(int productId, int buyerId,
                                   const QString& remark, int* outOrderId)
{
    QSqlDatabase& db = DbManager::db();

    // 事务外先读一遍，把三个能提前拒掉的情况挡在事务外面
    const auto p = m_productRepo->findById(productId);
    if (!p)
        return OpResult::fail(QStringLiteral("商品不存在"));
    if (p->status != Const::ProductStatus::OnSale)
        return OpResult::fail(QStringLiteral("该商品已售出或已下架"));
    if (p->sellerId == buyerId)
        return OpResult::fail(QStringLiteral("不能购买自己发布的商品"));

    if (!db.transaction())
        return OpResult::fail(QStringLiteral("开启事务失败"));

    // 1) 建订单（B 自己的表）
    Order o;
    o.productId = productId;
    o.buyerId   = buyerId;
    o.sellerId  = p->sellerId;
    o.amount    = p->price;
    o.remark    = remark;
    o.orderNo   = m_orderRepo->nextOrderNo();

    OpResult r = m_orderRepo->insert(o);
    if (!r.ok) {
        db.rollback();
        return r;
    }

    // 2) 商品置为已售出（A 的表）。条件更新：还在售才改得动，连点两次只有一次成功
    r = m_productRepo->updateStatus(productId, Const::ProductStatus::OnSale,
                                    Const::ProductStatus::Sold);
    if (!r.ok) {
        db.rollback();
        return OpResult::fail(QStringLiteral("该商品刚刚被买走了，请刷新"));
    }

    // 3) 写第一条状态日志（C 的表）
    TradeLog log;
    log.orderId    = o.id;
    log.fromStatus = QString();   // 下单之前没有状态
    log.toStatus   = Const::OrderStatus::PendingPay;
    log.action     = Const::TradeAction::Create;
    log.operatorId = buyerId;
    log.remark     = QStringLiteral("下单");

    r = m_logRepo->insert(log);
    if (!r.ok) {
        db.rollback();
        return r;
    }

    if (!db.commit()) {
        db.rollback();
        return OpResult::fail(QStringLiteral("提交事务失败"));
    }

    if (outOrderId)
        *outOrderId = o.id;
    emit orderCreated(o.id);
    return OpResult::success(QString::number(o.id));
}

QVector<Order> OrderService::queryOrders(const OrderQuery& q)
{
    return m_orderRepo->search(q);
}

std::optional<Order> OrderService::findOrder(int orderId)
{
    return m_orderRepo->findById(orderId);
}
