#pragma once
#include <QObject>
#include <QSqlQuery>

#include "core/repository/i_product_repository.h"

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
    QVector<CategoryNode> categories() override;

private:
    static Product mapRow(const QSqlQuery& q);
};
