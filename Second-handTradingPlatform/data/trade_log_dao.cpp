#include "data/trade_log_dao.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "data/db_manager.h"

namespace {

// 把一行查询结果翻译成 TradeLog 实体（列名 → 字段名一一对应）
TradeLog rowToLog(const QSqlQuery& q)
{
    TradeLog l;
    l.id           = q.value("id").toInt();
    l.orderId      = q.value("order_id").toInt();
    l.fromStatus   = q.value("from_status").toString();
    l.toStatus     = q.value("to_status").toString();
    l.action       = q.value("action").toString();
    l.operatorId   = q.value("operator_id").toInt();
    l.remark       = q.value("remark").toString();
    l.createdAt    = q.value("created_at").toString();
    l.operatorName = q.value("operator_name").toString();
    l.orderNo      = q.value("order_no").toString();
    l.productTitle = q.value("product_title").toString();
    return l;
}

} // namespace

OpResult TradeLogDao::insert(TradeLog& log)
{
    QSqlQuery q(DbManager::db());
    q.prepare("INSERT INTO trade_log(order_id, from_status, to_status, action, operator_id, remark) "
              "VALUES(?, ?, ?, ?, ?, ?)");
    q.addBindValue(log.orderId);
    q.addBindValue(log.fromStatus);   // 下单时传空串 ""
    q.addBindValue(log.toStatus);
    q.addBindValue(log.action);
    q.addBindValue(log.operatorId);
    q.addBindValue(log.remark);
    if (!q.exec())
        return OpResult::fail("写状态日志失败：" + q.lastError().text());
    log.id = q.lastInsertId().toInt();
    return OpResult::success();
}

QVector<TradeLog> TradeLogDao::findByOrderId(int orderId)
{
    QVector<TradeLog> result;
    QSqlQuery q(DbManager::db());
    q.prepare("SELECT l.*, u.nickname AS operator_name, "
              "       o.order_no, p.title AS product_title "
              "FROM trade_log l "
              "LEFT JOIN users u    ON u.id = l.operator_id "
              "LEFT JOIN orders o   ON o.id = l.order_id "
              "LEFT JOIN products p ON p.id = o.product_id "
              "WHERE l.order_id = ? "
              "ORDER BY l.id ASC");   // 必须按 id 排：同一秒多条日志时间戳相同，按时间会乱序
    q.addBindValue(orderId);
    if (!q.exec())
        return result;
    while (q.next())
        result.append(rowToLog(q));
    return result;
}

QVector<TradeLog> TradeLogDao::search(const TradeLogQuery& cond)
{
    QVector<TradeLog> result;

    // 动态条件只拼结构，不拼值：值全部走 addBindValue，防 SQL 注入
    QString sql = "SELECT l.*, u.nickname AS operator_name, "
                  "       o.order_no, p.title AS product_title "
                  "FROM trade_log l "
                  "LEFT JOIN users u    ON u.id = l.operator_id "
                  "LEFT JOIN orders o   ON o.id = l.order_id "
                  "LEFT JOIN products p ON p.id = o.product_id "
                  "WHERE 1=1";
    QVariantList args;

    if (cond.orderId > 0) {
        sql += " AND l.order_id = ?";
        args << cond.orderId;
    }
    if (!cond.status.isEmpty()) {
        sql += " AND l.to_status = ?";
        args << cond.status;
    }
    if (!cond.keyword.isEmpty()) {
        sql += " AND (o.order_no LIKE ? OR l.remark LIKE ? OR u.nickname LIKE ?)";
        const QString k = "%" + cond.keyword + "%";
        args << k << k << k;
    }
    sql += " ORDER BY l.id DESC";   // 查询页：最新的一条在最上面

    QSqlQuery q(DbManager::db());
    q.prepare(sql);
    for (const QVariant& v : args)
        q.addBindValue(v);
    if (!q.exec())
        return result;
    while (q.next())
        result.append(rowToLog(q));
    return result;
}
