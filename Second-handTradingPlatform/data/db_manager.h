#pragma once

#include <QSqlDatabase>
#include <QString>

// 全项目唯一的数据库入口。所有 DAO 都用 DbManager::db() 拿连接。
class DbManager
{
public:
    // 创建 data 目录、打开 app.db、必要时建表。
    // 失败时把原因写进 errMsg（可为 nullptr），返回 false。
    static bool init(QString* errMsg = nullptr);

    // 唯一连接。第一次调用时初始化，之后一直复用。
    static QSqlDatabase& db();

    // 数据库文件的绝对路径（exe 同目录/data/app.db），日志和提示里用
    static QString dbPath();

    // 调试用：打印所有表名和分类条数，仅在 QT_DEBUG 下调用
    static void dumpSchema();
};