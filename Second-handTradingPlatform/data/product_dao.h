#pragma once
#include <QObject>
#include <QSqlQuery>

#include "core/repository/i_product_repository.h"

// 商品 DAO [A]：products 表的所有 SQL 只写在这个文件里。
class ProductDao : public QObject, public IProductRepository
{
    Q_OBJECT
public:
    explicit ProductDao(QObject* parent = nullptr);

    OpResult insert(Product& p) override;
    OpResult updateStatus(int id, const QString& from, const QString& to) override;
    OpResult updateInfo(const Product& p) override;
    std::optional<Product> findById(int id) override;
    QVector<Product> search(const ProductQuery& q) override;
    QVector<Product> findBySeller(int sellerId) override;

    // 分类下拉用：只读，按 父分类在前、子分类在后 的顺序返回
    QVector<CategoryNode> categories();

private:
    // 把一行（products + JOIN 出来的 seller_name / category_name）转成实体
    static Product mapRow(const QSqlQuery& query);
};
