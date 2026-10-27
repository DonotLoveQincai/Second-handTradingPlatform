#pragma once

#include <QHash>
#include <QString>

// 全项目只在这里出现状态/动作的字面量。别处一律用 Const:: 下的常量。
// 用 QString 而不是 enum：打开数据库能直接看懂 status 列，答辩好讲。
namespace Const {

namespace ProductStatus {
    inline const QString OnSale  = "ON_SALE";
    inline const QString OffSale = "OFF_SALE";
    inline const QString Sold    = "SOLD";
}

namespace OrderStatus {
    inline const QString PendingPay = "PENDING_PAY";
    inline const QString Paid       = "PAID";
    inline const QString Shipped    = "SHIPPED";
    inline const QString Completed  = "COMPLETED";
    inline const QString Cancelled  = "CANCELLED";
}

namespace TradeAction {
    inline const QString Create  = "CREATE";
    inline const QString Pay     = "PAY";
    inline const QString Ship    = "SHIP";
    inline const QString Confirm = "CONFIRM";
    inline const QString Cancel  = "CANCEL";
}

// 状态码 → 中文。界面只调这个，不要自己写 if/else 翻译。
inline QString zh(const QString& code)
{
    static const QHash<QString, QString> m = {
        { "ON_SALE",     "在售"   },
        { "OFF_SALE",    "已下架" },
        { "SOLD",        "已售出" },
        { "PENDING_PAY", "待付款" },
        { "PAID",        "已付款" },
        { "SHIPPED",     "已发货" },
        { "COMPLETED",   "已完成" },
        { "CANCELLED",   "已取消" },
    };
    return m.value(code, code);
}

} // namespace Const