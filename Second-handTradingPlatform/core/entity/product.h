#pragma once
#include <QString>
#include <QVector>

struct Product {
    int id=0, sellerId=0, categoryId=0;
    QString title, description, status, createdAt, updatedAt;
    double price=0.0;
    int conditionLevel=9;
    QString sellerName, categoryName;
};

struct ProductQuery {
    QString keyword;
    QVector<int> categoryIds;
    double priceMin=-1, priceMax=-1;
    QString sortBy = "time_desc";
};
