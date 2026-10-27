#include "data/db_manager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

namespace {

    // 逐条执行 SQL 脚本。规则：按 ';' 切分，跳过空语句和整行的 "--" 注释。
    bool runSqlScript(QSqlDatabase& db, const QString& resourcePath, QString* errMsg)
    {
        QFile f(resourcePath);
        if (!f.open(QIODevice::ReadOnly)) {
            if (errMsg) *errMsg = QStringLiteral("无法打开内置 SQL 脚本：%1").arg(resourcePath);
            return false;
        }

        QString text = QString::fromUtf8(f.readAll());
        f.close();

        // 先去掉整行注释，避免注释里的分号把语句切断
        QStringList cleanLines;
        const QStringList rawLines = text.split('\n');
        for (const QString& raw : rawLines) {
            const QString line = raw.trimmed();
            if (line.startsWith("--"))
                continue;
            cleanLines << raw;
        }
        text = cleanLines.join('\n');

        QSqlQuery q(db);
        const QStringList stmts = text.split(';', Qt::SkipEmptyParts);
        for (const QString& raw : stmts) {
            const QString stmt = raw.trimmed();
            if (stmt.isEmpty())
                continue;
            if (!q.exec(stmt)) {
                if (errMsg) {
                    *errMsg = QStringLiteral("建表失败：%1\n出错语句：%2")
                        .arg(q.lastError().text(), stmt.left(200));
                }
                return false;
            }
        }
        return true;
    }

} // namespace

QString DbManager::dbPath()
{
    // 用 exe 所在目录，不要用当前工作目录：VS 里调试时两者不同，这是经典坑
    return QCoreApplication::applicationDirPath() + QStringLiteral("/data/app.db");
}

QSqlDatabase& DbManager::db()
{
    static QSqlDatabase conn = QSqlDatabase::database(QStringLiteral("main"));
    return conn;
}

bool DbManager::init(QString* errMsg)
{
    const QString dir = QCoreApplication::applicationDirPath() + QStringLiteral("/data");
    if (!QDir().mkpath(dir)) {
        if (errMsg) *errMsg = QStringLiteral("无法创建数据目录：%1").arg(dir);
        return false;
    }

    QSqlDatabase conn = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
        QStringLiteral("main"));
    conn.setDatabaseName(dbPath());

    if (!conn.open()) {
        if (errMsg) {
            *errMsg = QStringLiteral("打开数据库失败：%1\n数据库路径：%2\n"
                "若提示 driver not loaded，请把 Qt 的 "
                "plugins/sqldrivers 目录拷到 exe 同目录。")
                .arg(conn.lastError().text(), dbPath());
        }
        return false;
    }

    // 这两个 PRAGMA 是"按连接"生效的，每次打开都要设
    QSqlQuery q(conn);
    q.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    q.exec(QStringLiteral("PRAGMA journal_mode = WAL"));

    // 判断是否已建表：有 users 表就认为初始化过了
    q.exec(QStringLiteral("SELECT COUNT(*) FROM sqlite_master "
        "WHERE type='table' AND name='users'"));
    const bool hasTables = q.next() && q.value(0).toInt() > 0;

    if (!hasTables) {
        if (!runSqlScript(conn, QStringLiteral(":/sql/001_init.sql"), errMsg))
            return false;
        qDebug() << "[DbManager] 数据库初始化完成：" << dbPath();
    }
    else {
        qDebug() << "[DbManager] 使用已有数据库：" << dbPath();
    }
    return true;
}

void DbManager::dumpSchema()
{
#ifdef QT_DEBUG
    QSqlDatabase& conn = db();
    qDebug() << "[DbManager] 表：" << conn.tables();
    QSqlQuery q(conn);
    if (q.exec(QStringLiteral("SELECT COUNT(*) FROM categories")) && q.next())
        qDebug() << "[DbManager] 分类条数：" << q.value(0).toInt();
    if (q.exec(QStringLiteral("SELECT id,name,parent_id FROM categories ORDER BY id"))) {
        while (q.next()) {
            qDebug() << "  category" << q.value(0).toInt()
                << q.value(1).toString()
                << "parent=" << q.value(2).toString();
        }
    }
#endif
}
