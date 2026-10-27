#pragma once

#include <optional>

#include <QString>
#include <QVector>

#include "core/common/opresult.h"
#include "core/entity/product.h"

// 商品仓储接口：A 域实现，B 下单、C 取消都要调 updateStatus。
class IProductRepository
{
public:
    virtual ~IProductRepository() = default;

    // 成功后回填 p.id
    virtual OpResult insert(Product& p) = 0;

    // 条件更新：只有当前状态 == from 才改成 to，没改到（影响行数 0）返回失败。
    // 全项目四种用法：上架 (OFF_SALE→ON_SALE)、下架 (ON_SALE→OFF_SALE)、
    // 下单占用 (ON_SALE→SOLD)、取消订单释放 (SOLD→ON_SALE)。
    virtual OpResult updateStatus(int id, const QString& from, const QString& to) = 0;

    // 改信息（标题/描述/价格/分类）：只能改自己的、且不能改已售出的
    virtual OpResult updateInfo(const Product& p) = 0;

    virtual std::optional<Product> findById(int id) = 0;
    virtual QVector<Product> search(const ProductQuery& q) = 0;
    virtual QVector<Product> findBySeller(int sellerId) = 0;
};
