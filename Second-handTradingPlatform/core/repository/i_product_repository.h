#pragma once
#include <optional>
#include <QVector>

#include "core/common/opresult.h"
#include "core/entity/category.h"
#include "core/entity/product.h"

class IProductRepository {
public:
    virtual ~IProductRepository() = default;
    virtual OpResult insert(Product& p) = 0;
    virtual OpResult updateStatus(int id, const QString& from, const QString& to) = 0;
    virtual OpResult updateInfo(const Product& p) = 0;
    virtual std::optional<Product> findById(int id) = 0;
    virtual QVector<Product> search(const ProductQuery& q) = 0;
    virtual QVector<Product> findBySeller(int sellerId) = 0;
    virtual QVector<Category> categories() = 0;
};
