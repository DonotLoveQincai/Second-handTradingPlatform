#pragma once
#include <QString>

inline QString conditionText(int lv)
{
    if (lv >= 10)
        return "全新";
    return QString::number(lv) + "成新";
}
