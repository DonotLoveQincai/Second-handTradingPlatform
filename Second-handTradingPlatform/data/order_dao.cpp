#include "data/order_dao.h"

#include <QDate>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariantList>

#include "core/common/constants.h"
#include "data/db_manager.h"

namespace {

// 买到的和卖出的共用这一条，ORDER BY 放最后拼
const char* kSelect =
    "SELECT o.id, o.order_no, o.product_id, o.buyer_id, o.seller_id, o.amount, o.status, "
    "       o.remark, o.created_at, o.updated_at, "
    "       p.title AS product_title, b.nickname AS buyer_name, s.nickname AS seller_name "
    "FROM orders o "
    "LEFT JOIN products p ON p.id = o.product_id "
    "LEFT JOIN users b ON b.id = o.buyer_id "
    "LEFT JOIN users s ON s.id = o.seller_id";

Order readOrder(const QSqlQuery& q)
{
    Order o;
    o.id           = q.value("id").toInt();
    o.orderNo      = q.value("order_no").toString();
    o.productId    = q.value("product_id").toInt();
    o.buyerId      = q.value("buyer_id").toInt();
    o.sellerId     = q.value("seller_id").toInt();
    o.amount       = q.value("amount").toDouble();
    o.status       = q.value("status").toString();
    o.remark       = q.value("remark").toString();
    o.createdAt    = q.value("created_at").toString();
    o.updatedAt    = q.value("updated_at").toString();
    o.productTitle = q.value("product_title").toString();
    o.buyerName    = q.value("buyer_name").toString();
    o.sellerName   = q.value("seller_name").toString();
    return o;
}

} // namespace

QString OrderDao::nextOrderNo()
{
    const QString day = QDate::currentDate().toString("yyyyMMdd");

    QSqlQuery q(DbManager::db());
    q.prepare("SELECT COUNT(*) FROM orders WHERE order_no LIKE ?");
    q.addBindValue(day + "-%");
    q.exec();
    q.next();

    const int n = q.value(0).toInt() + 1;
    return day + "-" + QString("%1").arg(n, 4, 10, QChar('0'));
}

OpResult OrderDao::insert(Order& o)
{
    QSqlQuery q(DbManager::db());
    q.prepare("INSERT INTO orders(order_no, product_id, buyer_id, seller_id, amount, status, remark) "
              "VALUES(?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(o.orderNo);
    q.addBindValue(o.productId);
    q.addBindValue(o.buyerId);
    q.addBindValue(o.sellerId);
    q.addBindValue(o.amount);
    q.addBindValue(Const::OrderStatus::PendingPay);
    q.addBindValue(o.remark);

    if (!q.exec())
        return OpResult::fail("创建订单失败：" + q.lastError().text());

    o.id = q.lastInsertId().toInt();
    o.status = Const::OrderStatus::PendingPay;
    return OpResult::success(QString::number(o.id));
}

// 条件更新：只有当前还是 from 才动，改不到说明别人先改了
OpResult OrderDao::updateStatus(int orderId, const QString& from, const QString& to)
{
    QSqlQuery q(DbManager::db());
    q.prepare("UPDATE orders SET status = ?, updated_at = datetime('now','localtime') "
              "WHERE id = ? AND status = ?");
    q.addBindValue(to);
    q.addBindValue(orderId);
    q.addBindValue(from);

    if (!q.exec())
        return OpResult::fail("更新订单状态失败：" + q.lastError().text());
    if (q.numRowsAffected() != 1)
        return OpResult::fail("订单状态已变，请刷新");

    return OpResult::success(QString::number(orderId));
}

std::optional<Order> OrderDao::findById(int id)
{
    QSqlQuery q(DbManager::db());
    q.prepare(QString(kSelect) + " WHERE o.id = ?");
    q.addBindValue(id);
    if (!q.exec() || !q.next())
        return std::nullopt;
    return readOrder(q);
}

QVector<Order> OrderDao::search(const OrderQuery& cond)
{
    QString sql = QString(kSelect) + " WHERE 1 = 1";
    QVariantList binds;

    if (cond.buyerId > 0) {
        sql += " AND o.buyer_id = ?";
        binds << cond.buyerId;
    }
    if (cond.sellerId > 0) {
        sql += " AND o.seller_id = ?";
        binds << cond.sellerId;
    }
    if (!cond.status.isEmpty()) {
        sql += " AND o.status = ?";
        binds << cond.status;
    }
    if (!cond.keyword.isEmpty()) {
        sql += " AND (o.order_no LIKE ? OR p.title LIKE ?)";
        const QString like = "%" + cond.keyword + "%";
        binds << like << like;
    }
    sql += " ORDER BY o.created_at DESC, o.id DESC";

    QSqlQuery q(DbManager::db());
    q.prepare(sql);
    for (const QVariant& v : binds)
        q.addBindValue(v);

    QVector<Order> out;
    if (!q.exec()) {
        qWarning() << "OrderDao::search failed:" << q.lastError().text();
        return out;
    }
    while (q.next())
        out.append(readOrder(q));
    return out;
}
