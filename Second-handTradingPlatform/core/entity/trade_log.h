#pragma once

#include <QString>
#include <QVector>

// 交易日志实体：订单每变一次状态，就多一条记录。
// 字段名和数据库 trade_log 表的列名一一对应；末尾三个 xxxName 是 JOIN 查出来给界面显示的，不存库。
struct TradeLog
{
    int id = 0;
    int orderId = 0;
    int operatorId = 0;
    QString fromStatus;   // 变更前的状态，下单那一条是空串 ""
    QString toStatus;     // 变更后的状态
    QString action;       // CREATE / PAY / SHIP / CONFIRM / CANCEL
    QString remark;       // 动作的中文名，界面直接显示
    QString createdAt;

    QString orderNo;      // JOIN 填充
    QString productTitle; // JOIN 填充
    QString operatorName; // JOIN 填充
};

// 交易记录查询条件：字段全空 = 查全平台流水
struct TradeLogQuery
{
    int orderId = 0;      // > 0 只看这一单
    QString status;       // 按 to_status 过滤，空 = 不限
    QString keyword;      // 订单号 / 操作人 / 备注 模糊匹配，空 = 不限
};
