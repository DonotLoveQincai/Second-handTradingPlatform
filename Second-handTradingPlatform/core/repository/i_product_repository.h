#pragma once
#include <optional>
#include <QVector>

#include "core/common/opresult.h"
#include "core/entity/product.h"

// 商品仓储接口 [A]。SQL 只允许出现在 data/ 的实现里。
class IProductRepository {
public:
    virtual ~IProductRepository() = default;

    virtual OpResult insert(Product& p) = 0;

    // 条件更新：只有当前状态 == from 才改成 to，没改到就返回失败。
    // 一条 UPDATE ... WHERE id=? AND status=? 同时表达“改成什么”和“只有原状态是这个才改”，
    // 所以连点两次购买只有一次成功。
    virtual OpResult updateStatus(int id, const QString& from, const QString& to) = 0;

    // 改信息（标题/描述/价格/分类），只能改自己的、且不能改已售出的
    virtual OpResult updateInfo(const Product& p) = 0;

    virtual std::optional<Product> findById(int id) = 0;
    virtual QVector<Product> search(const ProductQuery& q) = 0;
    virtual QVector<Product> findBySeller(int sellerId) = 0;

    // 契约补漏：检索页和发布框都要分类列表，但契约里没有分类仓储。
    // 分类检索本来就是商品域的一部分，所以放在这里，SQL 依然只在 data/ 里。
    virtual QVector<CategoryNode> categories() = 0;
};
