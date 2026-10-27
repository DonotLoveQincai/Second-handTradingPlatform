#pragma once

#include <QString>
#include <QVector>

// 商品实体：字段名和 products 表列名一致；sellerName/categoryName 是 JOIN 查出来给界面显示的，不存库。
struct Product
{
    int     id = 0;
    int     sellerId = 0;
    int     categoryId = 0;
    QString title;
    QString description;
    QString status;
    QString createdAt;
    QString updatedAt;
    double  price = 0.0;
    int     conditionLevel = 9;   // 1~10，10 = 全新

    QString sellerName;    // JOIN 填充
    QString categoryName;  // JOIN 填充
};

// 商品检索条件：字段全空 = 不限
struct ProductQuery
{
    QString     keyword;            // 标题/描述模糊匹配，空 = 不限
    QVector<int> categoryIds;       // 分类，空 = 全部
    double      priceMin = -1;      // < 0 = 不限
    double      priceMax = -1;      // < 0 = 不限
    QString     sortBy = "time_desc";   // time_desc | price_asc | price_desc
};
