#include "services/product_service.h"

#include <QString>

#include "core/common/constants.h"
#include "core/repository/i_product_repository.h"

ProductService::ProductService(IProductRepository* repo, QObject* parent)
    : QObject(parent)
    , m_repo(repo)
{
}

// 发布 / 修改共用的字段校验：都在这里，别在两处各写一遍
OpResult ProductService::validate(const Product& p) const
{
    if (p.title.trimmed().isEmpty())
        return OpResult::fail(QStringLiteral("请填写标题"));
    if (p.title.size() > 60)
        return OpResult::fail(QStringLiteral("标题不能超过 60 字"));
    if (p.price <= 0)
        return OpResult::fail(QStringLiteral("价格必须大于 0"));
    if (p.categoryId <= 0)
        return OpResult::fail(QStringLiteral("请选择分类"));
    if (p.conditionLevel < 1 || p.conditionLevel > 10)
        return OpResult::fail(QStringLiteral("成色只能是 1~10"));
    return OpResult::success();
}

OpResult ProductService::publish(Product& p, int sellerId)
{
    p.title = p.title.trimmed();
    const OpResult invalid = validate(p);
    if (!invalid.ok)
        return invalid;

    p.sellerId = sellerId;                          // 卖家不允许由界面指定
    p.status = Const::ProductStatus::OnSale;

    const OpResult result = m_repo->insert(p);
    if (result.ok)
        emit productsChanged();                     // 让检索页 / 我的商品页重查
    return result;
}

OpResult ProductService::updateInfo(Product p, int sellerId)
{
    const std::optional<Product> old = m_repo->findById(p.id);
    if (!old)
        return OpResult::fail(QStringLiteral("商品不存在"));
    if (old->sellerId != sellerId)
        return OpResult::fail(QStringLiteral("只能修改自己发布的商品"));
    if (old->status == Const::ProductStatus::Sold)
        return OpResult::fail(QStringLiteral("已售出的商品不能修改"));

    p.title = p.title.trimmed();
    const OpResult invalid = validate(p);
    if (!invalid.ok)
        return invalid;

    p.sellerId = sellerId;                          // 卖家不允许被改

    const OpResult result = m_repo->updateInfo(p);
    if (result.ok)
        emit productsChanged();
    return result;
}

OpResult ProductService::takeDown(int productId, int sellerId)
{
    const std::optional<Product> product = m_repo->findById(productId);
    if (!product)
        return OpResult::fail(QStringLiteral("商品不存在"));
    if (product->sellerId != sellerId)
        return OpResult::fail(QStringLiteral("只能下架自己发布的商品"));
    if (product->status == Const::ProductStatus::Sold)
        return OpResult::fail(QStringLiteral("已售出的商品不能下架"));

    const OpResult result = m_repo->updateStatus(
        productId, Const::ProductStatus::OnSale, Const::ProductStatus::OffSale);
    if (result.ok)
        emit productsChanged();
    return result;
}

// 契约 §7.1 的四种 updateStatus 用法里，“上架”也是 A 的，补一个入口
OpResult ProductService::putOnSale(int productId, int sellerId)
{
    const std::optional<Product> product = m_repo->findById(productId);
    if (!product)
        return OpResult::fail(QStringLiteral("商品不存在"));
    if (product->sellerId != sellerId)
        return OpResult::fail(QStringLiteral("只能上架自己发布的商品"));
    if (product->status == Const::ProductStatus::Sold)
        return OpResult::fail(QStringLiteral("已售出的商品不能上架"));

    const OpResult result = m_repo->updateStatus(
        productId, Const::ProductStatus::OffSale, Const::ProductStatus::OnSale);
    if (result.ok)
        emit productsChanged();
    return result;
}

QVector<Product> ProductService::search(const ProductQuery& query)
{
    return m_repo->search(query);
}

QVector<Product> ProductService::myProducts(int sellerId)
{
    return m_repo->findBySeller(sellerId);
}

std::optional<Product> ProductService::find(int productId)
{
    return m_repo->findById(productId);
}

QVector<CategoryNode> ProductService::categories()
{
    return m_repo->categories();
}
