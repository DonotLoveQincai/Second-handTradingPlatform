#pragma once

#include <QString>

// 所有写操作的统一返回值。项目不用异常，也不用 std::expected。

struct OpResult
{
    bool    ok = false;
    QString message;   // 失败原因，直接显示给用户
    QString value;     // 成功时可带一个值（新 id、订单号），不需要就留空

    static OpResult success(const QString& v = QString())
    {
        OpResult r;
        r.ok = true;
        r.value = v;
        return r;
    }

    static OpResult fail(const QString& m)
    {
        OpResult r;
        r.ok = false;
        r.message = m;
        return r;
    }
};