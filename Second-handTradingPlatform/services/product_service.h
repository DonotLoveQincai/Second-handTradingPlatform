#pragma once
#include <QObject>
#include <optional>
#include <QVector>

#include "core/common/opresult.h"
#include "core/entity/product.h"

class IProductRepository;

// 商品业务 [A]：校验规则放这里，DAO 只管 SQL，界面只管显示。
class ProductService : public QObject
{
    Q_OBJECT
public:
    explicit ProductService(IProductRepository* repo, QObject* parent = nullptr);

    OpResult publish(Product& p, int sellerId);      // 校验标题/价格/分类
    OpResult updateInfo(Product p, int sellerId);    // 改信息：只能改自己的，已售出不能改
    OpResult takeDown(int productId, int sellerId);  // 只有自己的商品能下架
    // 契约 §7.1 的用法表里有“上架 (OFF_SALE→ON_SALE)”，§7.2 的签名表漏了这一条，A 补上
    OpResult putOnSale(int productId, int sellerId);

    // 页面不能直接碰 DAO（铁律 1：ui → services → data），所以查询也走这里
    QVector<Product> search(const ProductQuery& query);
    QVector<Product> myProducts(int sellerId);
    std::optional<Product> find(int productId);
    QVector<CategoryNode> categories();              // 检索页 / 发布框的分类下拉

signals:
    void productsChanged();                          // 让检索页/我的商品页刷新

private:
    OpResult validate(const Product& p) const;

    IProductRepository* m_repo = nullptr;
};
