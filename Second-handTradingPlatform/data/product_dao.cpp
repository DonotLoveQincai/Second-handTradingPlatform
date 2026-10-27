#include "data/product_dao.h"

#include <QSqlError>
#include <QVariant>

#include "core/common/constants.h"
#include "data/db_manager.h"

namespace {
const char* SQL_FIELDS =
    "SELECT p.*, u.nickname AS seller_name, c.name AS category_name "
    "FROM products p "
    "LEFT JOIN users u ON u.id = p.seller_id "
    "LEFT JOIN categories c ON c.id = p.category_id ";
}

ProductDao::ProductDao(QObject* parent) : QObject(parent)
{
}

Product ProductDao::mapRow(const QSqlQuery& q)
{
    Product p;
    p.id = q.value("id").toInt();
    p.sellerId = q.value("seller_id").toInt();
    p.categoryId = q.value("category_id").toInt();
    p.title = q.value("title").toString();
    p.description = q.value("description").toString();
    p.price = q.value("price").toDouble();
    p.conditionLevel = q.value("condition_level").toInt();
    p.status = q.value("status").toString();
    p.createdAt = q.value("created_at").toString();
    p.updatedAt = q.value("updated_at").toString();
    p.sellerName = q.value("seller_name").toString();
    p.categoryName = q.value("category_name").toString();
    return p;
}

OpResult ProductDao::insert(Product& p)
{
    QSqlQuery q(DbManager::db());
    q.prepare("INSERT INTO products(seller_id, category_id, title, description, price, "
              "condition_level, status) VALUES(?,?,?,?,?,?,?)");
    q.addBindValue(p.sellerId);
    q.addBindValue(p.categoryId);
    q.addBindValue(p.title);
    q.addBindValue(p.description);
    q.addBindValue(p.price);
    q.addBindValue(p.conditionLevel);
    q.addBindValue(p.status.isEmpty() ? Const::ProductStatus::OnSale : p.status);
    if (!q.exec())
        return OpResult::fail(QString("商品没存进去：") + q.lastError().text());
    p.id = q.lastInsertId().toInt();
    return OpResult::success(QString::number(p.id));
}

OpResult ProductDao::updateStatus(int id, const QString& from, const QString& to)
{
    QSqlQuery q(DbManager::db());
    q.prepare("UPDATE products SET status=?, updated_at=datetime('now','localtime') "
              "WHERE id=? AND status=?");
    q.addBindValue(to);
    q.addBindValue(id);
    q.addBindValue(from);
    if (!q.exec())
        return OpResult::fail(QString("改状态出错：") + q.lastError().text());
    if (q.numRowsAffected() != 1)
        return OpResult::fail("状态已经变了，刷新一下再试");
    return OpResult::success();
}

OpResult ProductDao::updateInfo(const Product& p)
{
    QSqlQuery q(DbManager::db());
    q.prepare("UPDATE products SET title=?, category_id=?, description=?, price=?, "
              "condition_level=?, updated_at=datetime('now','localtime') "
              "WHERE id=? AND status <> ?");
    q.addBindValue(p.title);
    q.addBindValue(p.categoryId);
    q.addBindValue(p.description);
    q.addBindValue(p.price);
    q.addBindValue(p.conditionLevel);
    q.addBindValue(p.id);
    q.addBindValue(Const::ProductStatus::Sold);
    if (!q.exec())
        return OpResult::fail(QString("改商品出错：") + q.lastError().text());
    return OpResult::success();
}

std::optional<Product> ProductDao::findById(int id)
{
    QSqlQuery q(DbManager::db());
    q.prepare(QString::fromLatin1(SQL_FIELDS) + "WHERE p.id = ?");
    q.addBindValue(id);
    if (!q.exec() || !q.next())
        return std::nullopt;
    return mapRow(q);
}

QVector<Product> ProductDao::search(const ProductQuery& query)
{
    QString sql = QString::fromLatin1(SQL_FIELDS) + "WHERE p.status = ?";
    QVariantList vals;
    vals << Const::ProductStatus::OnSale;

    if (!query.keyword.isEmpty()) {
        sql += " AND p.title LIKE ?";
        vals << ("%" + query.keyword + "%");
    }

    if (!query.categoryIds.isEmpty()) {
        sql += " AND p.category_id IN (";
        for (int i = 0; i < query.categoryIds.size(); i++) {
            if (i > 0)
                sql += ",";
            sql += "?";
        }
        sql += ")";
        for (int id : query.categoryIds)
            vals << id;
    }

    if (query.priceMin >= 0) {
        sql += " AND p.price >= ?";
        vals << query.priceMin;
    }
    if (query.priceMax >= 0) {
        sql += " AND p.price <= ?";
        vals << query.priceMax;
    }

    if (query.sortBy == "price_asc")
        sql += " ORDER BY p.price ASC";
    else if (query.sortBy == "price_desc")
        sql += " ORDER BY p.price DESC";
    else
        sql += " ORDER BY p.created_at DESC";

    QSqlQuery q(DbManager::db());
    q.prepare(sql);
    for (int i = 0; i < vals.size(); i++)
        q.addBindValue(vals[i]);

    QVector<Product> list;
    if (!q.exec())
        return list;
    while (q.next())
        list.append(mapRow(q));
    return list;
}

QVector<Product> ProductDao::findBySeller(int sellerId)
{
    QSqlQuery q(DbManager::db());
    q.prepare(QString::fromLatin1(SQL_FIELDS)
              + "WHERE p.seller_id = ? ORDER BY p.created_at DESC");
    q.addBindValue(sellerId);

    QVector<Product> list;
    if (!q.exec())
        return list;
    while (q.next())
        list.append(mapRow(q));
    return list;
}

QVector<CategoryNode> ProductDao::categories()
{
    QSqlQuery q(DbManager::db());
    q.exec("SELECT id, name, parent_id FROM categories "
           "ORDER BY CASE WHEN parent_id IS NULL THEN 0 ELSE 1 END, id");

    QVector<CategoryNode> list;
    while (q.next()) {
        CategoryNode n;
        n.id = q.value(0).toInt();
        n.name = q.value(1).toString();
        n.parentId = q.value(2).isNull() ? 0 : q.value(2).toInt();
        list.append(n);
    }
    return list;
}
