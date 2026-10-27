#include "data/product_dao.h"

#include <QHash>
#include <QSqlError>
#include <QStringList>
#include <QVariant>

#include "core/common/constants.h"
#include "data/db_manager.h"

namespace {
// 详情 / 检索 / 我的商品都要 JOIN 出卖家昵称和分类名，界面才能直接显示
const char* const kSelectWithNames =
    "SELECT p.*, u.nickname AS seller_name, c.name AS category_name "
    "FROM products p "
    "LEFT JOIN users u ON u.id = p.seller_id "
    "LEFT JOIN categories c ON c.id = p.category_id ";
}

ProductDao::ProductDao(QObject* parent)
    : QObject(parent)
{
}

Product ProductDao::mapRow(const QSqlQuery& query)
{
    // 一律用列名取值（不用序号），以后加列不会错位
    Product p;
    p.id             = query.value(QStringLiteral("id")).toInt();
    p.sellerId       = query.value(QStringLiteral("seller_id")).toInt();
    p.categoryId     = query.value(QStringLiteral("category_id")).toInt();
    p.title          = query.value(QStringLiteral("title")).toString();
    p.description    = query.value(QStringLiteral("description")).toString();
    p.price          = query.value(QStringLiteral("price")).toDouble();
    p.conditionLevel = query.value(QStringLiteral("condition_level")).toInt();
    p.status         = query.value(QStringLiteral("status")).toString();
    p.createdAt      = query.value(QStringLiteral("created_at")).toString();
    p.updatedAt      = query.value(QStringLiteral("updated_at")).toString();
    p.sellerName     = query.value(QStringLiteral("seller_name")).toString();
    p.categoryName   = query.value(QStringLiteral("category_name")).toString();
    return p;
}

OpResult ProductDao::insert(Product& p)
{
    QSqlQuery q(DbManager::db());
    q.prepare(QStringLiteral(
        "INSERT INTO products(seller_id, category_id, title, description, price, "
        "condition_level, status) VALUES(?,?,?,?,?,?,?)"));
    q.addBindValue(p.sellerId);
    q.addBindValue(p.categoryId);
    q.addBindValue(p.title);
    q.addBindValue(p.description);
    q.addBindValue(p.price);
    q.addBindValue(p.conditionLevel);
    q.addBindValue(p.status.isEmpty() ? Const::ProductStatus::OnSale : p.status);

    if (!q.exec())
        return OpResult::fail(QStringLiteral("发布失败：") + q.lastError().text());

    p.id = q.lastInsertId().toInt();
    return OpResult::success(QString::number(p.id));
}

// 全项目最关键的 8 行：一条 UPDATE 同时表达“改成什么”和“只有原状态是这个才改”。
// 上架 / 下架 / 下单占用 / 取消释放 四种场景都调它，所以不需要写四个函数。
OpResult ProductDao::updateStatus(int id, const QString& from, const QString& to)
{
    QSqlQuery q(DbManager::db());
    q.prepare(QStringLiteral(
        "UPDATE products SET status=?, updated_at=datetime('now','localtime') "
        "WHERE id=? AND status=?"));
    q.addBindValue(to);       // 第 1 个 ?
    q.addBindValue(id);       // 第 2 个 ?
    q.addBindValue(from);     // 第 3 个 ?

    if (!q.exec())
        return OpResult::fail(QStringLiteral("更新商品状态失败：") + q.lastError().text());
    if (q.numRowsAffected() != 1)
        return OpResult::fail(QStringLiteral("商品状态已变，请刷新后重试"));
    return OpResult::success();
}

// 改信息不动 status，也不检查 numRowsAffected：
// SQLite 在“新值和旧值完全一样”时影响行数可能是 0 也可能是 1，拿它判断会误报。
// 已售出的商品由 SQL 里那句 status<>'SOLD' 挡住（数据库层的最后一道门）。
OpResult ProductDao::updateInfo(const Product& p)
{
    QSqlQuery q(DbManager::db());
    q.prepare(QStringLiteral(
        "UPDATE products SET title=?, category_id=?, description=?, price=?, "
        "condition_level=?, updated_at=datetime('now','localtime') "
        "WHERE id=? AND status <> ?"));
    q.addBindValue(p.title);
    q.addBindValue(p.categoryId);
    q.addBindValue(p.description);
    q.addBindValue(p.price);
    q.addBindValue(p.conditionLevel);
    q.addBindValue(p.id);
    q.addBindValue(Const::ProductStatus::Sold);

    if (!q.exec())
        return OpResult::fail(QStringLiteral("修改商品失败：") + q.lastError().text());
    return OpResult::success();
}

std::optional<Product> ProductDao::findById(int id)
{
    QSqlQuery q(DbManager::db());
    q.prepare(QString::fromLatin1(kSelectWithNames) + QStringLiteral("WHERE p.id = ?"));
    q.addBindValue(id);
    if (!q.exec() || !q.next())
        return std::nullopt;
    return mapRow(q);
}

// 检索：只拼结构、不拼值；排序字段只查白名单（用户输入绝不进 SQL）
QVector<Product> ProductDao::search(const ProductQuery& query)
{
    QString sql = QString::fromLatin1(kSelectWithNames);
    QVariantList binds;

    // 检索页只看在售商品：已下架 / 已售出的搜不到
    sql += QStringLiteral("WHERE p.status = ?");
    binds << Const::ProductStatus::OnSale;

    if (!query.keyword.isEmpty()) {
        sql += QStringLiteral(" AND p.title LIKE ?");
        binds << (QStringLiteral("%") + query.keyword + QStringLiteral("%"));
    }

    if (!query.categoryIds.isEmpty()) {
        QStringList placeholders;
        for (int i = 0; i < query.categoryIds.size(); ++i)
            placeholders << QStringLiteral("?");
        sql += QStringLiteral(" AND p.category_id IN (") + placeholders.join(QLatin1Char(',')) + QLatin1Char(')');
        for (int id : query.categoryIds)
            binds << id;
    }

    if (query.priceMin >= 0) {
        sql += QStringLiteral(" AND p.price >= ?");
        binds << query.priceMin;
    }
    if (query.priceMax >= 0) {
        sql += QStringLiteral(" AND p.price <= ?");
        binds << query.priceMax;
    }

    static const QHash<QString, QString> orderMap = {
        {QStringLiteral("time_desc"),  QStringLiteral("p.created_at DESC")},
        {QStringLiteral("price_asc"),  QStringLiteral("p.price ASC")},
        {QStringLiteral("price_desc"), QStringLiteral("p.price DESC")},
    };
    sql += QStringLiteral(" ORDER BY ")
           + orderMap.value(query.sortBy, QStringLiteral("p.created_at DESC"));

    QSqlQuery q(DbManager::db());
    q.prepare(sql);
    for (const QVariant& value : binds)
        q.addBindValue(value);

    QVector<Product> products;
    if (!q.exec())
        return products;

    while (q.next())
        products.append(mapRow(q));
    return products;
}

QVector<Product> ProductDao::findBySeller(int sellerId)
{
    QSqlQuery q(DbManager::db());
    q.prepare(QString::fromLatin1(kSelectWithNames)
              + QStringLiteral("WHERE p.seller_id = ? ORDER BY p.created_at DESC"));
    q.addBindValue(sellerId);

    QVector<Product> products;
    if (!q.exec())
        return products;

    while (q.next())
        products.append(mapRow(q));
    return products;
}

QVector<CategoryNode> ProductDao::categories()
{
    QSqlQuery q(DbManager::db());
    // 父分类在前、子分类在后（parent_id 为 NULL 的排前面）
    q.exec(QStringLiteral(
        "SELECT id, name, parent_id FROM categories "
        "ORDER BY CASE WHEN parent_id IS NULL THEN 0 ELSE 1 END, id"));

    QVector<CategoryNode> nodes;
    while (q.next()) {
        CategoryNode node;
        node.id = q.value(0).toInt();
        node.name = q.value(1).toString();
        node.parentId = q.value(2).isNull() ? 0 : q.value(2).toInt();
        nodes.append(node);
    }
    return nodes;
}
