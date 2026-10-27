#include "services/product_service.h"

#include "core/common/constants.h"
#include "core/repository/i_product_repository.h"

ProductService::ProductService(IProductRepository* repo, QObject* parent)
    : QObject(parent), m_repo(repo)
{
}

OpResult ProductService::publish(Product& p, int sellerId)
{
    p.title = p.title.trimmed();
    if (p.title.isEmpty())
        return OpResult::fail("标题还没写");
    if (p.title.size() > 60)
        return OpResult::fail("标题太长了，60 个字以内");
    if (p.price <= 0)
        return OpResult::fail("价格得大于 0");
    if (p.categoryId <= 0)
        return OpResult::fail("还没选分类");
    if (p.conditionLevel < 1 || p.conditionLevel > 10)
        return OpResult::fail("成色只能填 1 到 10");

    p.sellerId = sellerId;
    p.status = Const::ProductStatus::OnSale;

    OpResult r = m_repo->insert(p);
    if (r.ok)
        emit productsChanged();
    return r;
}

OpResult ProductService::updateInfo(Product p, int sellerId)
{
    std::optional<Product> old = m_repo->findById(p.id);
    if (!old)
        return OpResult::fail("找不到这个商品");
    if (old->sellerId != sellerId)
        return OpResult::fail("这不是你发布的商品");
    if (old->status == Const::ProductStatus::Sold)
        return OpResult::fail("已经卖掉了，改不了");

    p.title = p.title.trimmed();
    if (p.title.isEmpty())
        return OpResult::fail("标题还没写");
    if (p.title.size() > 60)
        return OpResult::fail("标题太长了，60 个字以内");
    if (p.price <= 0)
        return OpResult::fail("价格得大于 0");
    if (p.categoryId <= 0)
        return OpResult::fail("还没选分类");
    if (p.conditionLevel < 1 || p.conditionLevel > 10)
        return OpResult::fail("成色只能填 1 到 10");

    p.sellerId = sellerId;

    OpResult r = m_repo->updateInfo(p);
    if (r.ok)
        emit productsChanged();
    return r;
}

OpResult ProductService::takeDown(int productId, int sellerId)
{
    std::optional<Product> p = m_repo->findById(productId);
    if (!p)
        return OpResult::fail("找不到这个商品");
    if (p->sellerId != sellerId)
        return OpResult::fail("只能下架自己的商品");
    if (p->status == Const::ProductStatus::Sold)
        return OpResult::fail("已经卖掉了，下不了架");

    OpResult r = m_repo->updateStatus(productId, Const::ProductStatus::OnSale,
                                      Const::ProductStatus::OffSale);
    if (r.ok)
        emit productsChanged();
    return r;
}

OpResult ProductService::putOnSale(int productId, int sellerId)
{
    std::optional<Product> p = m_repo->findById(productId);
    if (!p)
        return OpResult::fail("找不到这个商品");
    if (p->sellerId != sellerId)
        return OpResult::fail("只能上架自己的商品");
    if (p->status == Const::ProductStatus::Sold)
        return OpResult::fail("已经卖掉了，上不了架");

    OpResult r = m_repo->updateStatus(productId, Const::ProductStatus::OffSale,
                                      Const::ProductStatus::OnSale);
    if (r.ok)
        emit productsChanged();
    return r;
}

QVector<Product> ProductService::search(const ProductQuery& q)
{
    return m_repo->search(q);
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
