#include "services/trade_state_machine.h"

#include <QDebug>
#include <QHash>
#include <QtGlobal>

#include "core/common/constants.h"

namespace {

// 一张表表达全部规则：当前状态 → (动作 → 目标状态)。和契约 01-契约.md 第 8 节一字不差。
const QHash<QString, QHash<QString, QString>>& table()
{
    static const QHash<QString, QHash<QString, QString>> t = {
        { Const::OrderStatus::PendingPay, {
            { Const::TradeAction::Pay,    Const::OrderStatus::Paid      },
            { Const::TradeAction::Cancel, Const::OrderStatus::Cancelled },
        }},
        { Const::OrderStatus::Paid, {
            { Const::TradeAction::Ship,   Const::OrderStatus::Shipped   },
            { Const::TradeAction::Cancel, Const::OrderStatus::Cancelled },
        }},
        { Const::OrderStatus::Shipped, {
            { Const::TradeAction::Confirm, Const::OrderStatus::Completed },
        }},
        { Const::OrderStatus::Completed, {} },   // 终态，没有出边
        { Const::OrderStatus::Cancelled, {} },   // 终态，没有出边
    };
    return t;
}

} // namespace

bool TradeStateMachine::canTransit(const QString& from, const QString& action)
{
    return table().value(from).contains(action);
}

QString TradeStateMachine::nextStatus(const QString& from, const QString& action)
{
    return table().value(from).value(action);   // 非法时自动返回空串
}

QStringList TradeStateMachine::actionsFor(const QString& status)
{
    return table().value(status).keys();
}

QString TradeStateMachine::actionName(const QString& action)
{
    static const QHash<QString, QString> m = {
        { Const::TradeAction::Create,  QStringLiteral("创建订单") },
        { Const::TradeAction::Pay,     QStringLiteral("付款")     },
        { Const::TradeAction::Ship,    QStringLiteral("发货")     },
        { Const::TradeAction::Confirm, QStringLiteral("确认收货") },
        { Const::TradeAction::Cancel,  QStringLiteral("取消订单") },
    };
    return m.value(action, action);
}

QString TradeStateMachine::statusName(const QString& status)
{
    return Const::zh(status);   // constants.h 里已有状态码→中文，直接复用
}

void TradeStateMachine::selfCheck()
{
    int legal = 0, checked = 0;
    const QStringList allStatus = {
        Const::OrderStatus::PendingPay, Const::OrderStatus::Paid,
        Const::OrderStatus::Shipped, Const::OrderStatus::Completed,
        Const::OrderStatus::Cancelled,
    };
    const QStringList allAction = {
        Const::TradeAction::Pay, Const::TradeAction::Ship,
        Const::TradeAction::Confirm, Const::TradeAction::Cancel,
    };
    for (const QString& s : allStatus) {
        for (const QString& a : allAction) {
            ++checked;
            if (canTransit(s, a)) {
                Q_ASSERT(!nextStatus(s, a).isEmpty());
                ++legal;
            }
        }
    }
    Q_ASSERT(checked == 20);   // 少一个状态/动作都是表写错了
    Q_ASSERT(legal == 5);      // 合法边多一条少一条，Debug 下直接崩
    qDebug() << "[TradeStateMachine] 自检通过：" << legal << "/" << checked << "条合法转移";
}
