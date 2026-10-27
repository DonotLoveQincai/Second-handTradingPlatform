#pragma once
#include <QString>
#include <QVector>

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
    int conditionLevel = 9;
    QString sellerName;
    QString categoryName;
};

struct ProductQuery {
    QString keyword;
    QVector<int> categoryIds;
    double priceMin = -1;
    double priceMax = -1;
    QString sortBy = "time_desc";
};

struct CategoryNode {
    int id = 0;
    int parentId = 0;
    QString name;
};

inline QString conditionText(int lv)
{
    if (lv >= 10)
        return "全新";
    return QString::number(lv) + "成新";
}
