#pragma once
#include <QObject>
#include <optional>
#include <QVector>

#include "core/common/opresult.h"
#include "core/entity/category.h"
#include "core/entity/product.h"

class IProductRepository;

class ProductService : public QObject
{
    Q_OBJECT
public:
    explicit ProductService(IProductRepository* repo, QObject* parent = nullptr);

    OpResult publish(Product& p, int sellerId);
    OpResult updateInfo(Product p, int sellerId);
    OpResult takeDown(int productId, int sellerId);
    OpResult putOnSale(int productId, int sellerId);

    QVector<Product> search(const ProductQuery& q);
    QVector<Product> myProducts(int sellerId);
    std::optional<Product> find(int productId);
    QVector<Category> categories();

signals:
    void productsChanged();

private:
    IProductRepository* m_repo = nullptr;
};
