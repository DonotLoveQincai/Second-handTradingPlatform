#pragma once
#include <QString>
#include <QVector>

struct CategoryItem
{
    int id;
    int parentId;
    QString name;
};

inline QVector<CategoryItem> allCategories()
{
    return {
        {1, 0, "数码电子"},
        {2, 0, "图书教材"},
        {3, 0, "家居日用"},
        {4, 0, "服饰鞋包"},
        {5, 0, "运动户外"},
        {6, 1, "手机"},
        {7, 1, "电脑"},
        {8, 1, "耳机"},
    };
}

inline QString conditionText(int lv)
{
    if (lv >= 10)
        return "全新";
    return QString::number(lv) + "成新";
}
