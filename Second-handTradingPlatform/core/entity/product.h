#pragma once
#include <QString>
#include <QVector>

// 商品实体 [A]：字段名和 products 表列名一致；
// 末尾两个 xxxName 是 JOIN 出来给界面显示的，不存库。
struct Product {
    int id = 0;
    int sellerId = 0;
    int categoryId = 0;
    QString title;
    QString description;
    QString status;
    QString createdAt;
    QString updatedAt;
    double price = 0.0;
    int conditionLevel = 9;              // 1~10，10=全新
    QString sellerName;                  // JOIN 填充
    QString categoryName;                // JOIN 填充
};

// 检索条件：空 = 不限。没有分页、没有日期范围（契约砍了）。
struct ProductQuery {
    QString keyword;                     // 空=不限
    QVector<int> categoryIds;            // 空=全部分类
    double priceMin = -1;                // <0=不限
    double priceMax = -1;
    QString sortBy = "time_desc";        // time_desc | price_asc | price_desc
};

// 分类下拉框用。契约里没有分类实体/仓储，但检索页和发布框都要分类列表；
// 分类本来就是商品域的事（分类检索），所以由 A 的 ProductDao 查、ProductService 转发。
struct CategoryNode {
    int id = 0;
    int parentId = 0;                    // 0 = 顶级分类
    QString name;
};

// 成色 1~10 的显示文本：10 叫“全新”，其余叫“N成新”。
inline QString conditionText(int level)
{
    if (level >= 10)
        return QStringLiteral("全新");
    return QStringLiteral("%1成新").arg(level);
}

