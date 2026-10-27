#pragma once

#include <QString>

struct Order
{
    int     id = 0;
    int     productId = 0;
    int     buyerId = 0;
    int     sellerId = 0;
    QString orderNo;
    QString status;
    QString remark;
    QString createdAt;
    QString updatedAt;
    double  amount = 0.0;
    QString productTitle;   // 以下三个 JOIN 出来，不存库
    QString buyerName;
    QString sellerName;
};

struct OrderQuery
{
    int     buyerId = 0;    // 二选一，0 表示不限
    int     sellerId = 0;
    QString status;
    QString keyword;
};
