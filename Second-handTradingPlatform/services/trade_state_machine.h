#pragma once

#include <QString>
#include <QStringList>

// 交易状态机：全项目唯一定义"订单能怎么流转"的地方。
// 纯静态工具类，不 new、不碰数据库——答辩最好讲的部分。
class TradeStateMachine
{
public:
    // 当前状态 + 动作，能不能走
    static bool canTransit(const QString& from, const QString& action);

    // 合法时返回目标状态；非法返回空串（正好当"失败"用）
    static QString nextStatus(const QString& from, const QString& action);

    // 这个状态下能做的动作列表（订单详情页的按钮就是它给的）
    static QStringList actionsFor(const QString& status);

    // 动作码 → 中文（按钮文字用）
    static QString actionName(const QString& action);

    // 状态码 → 中文（表格显示用）
    static QString statusName(const QString& status);

    // 穷举自检：5 状态 × 4 动作 = 20 组，合法必须恰好 5 条
    static void selfCheck();
};
