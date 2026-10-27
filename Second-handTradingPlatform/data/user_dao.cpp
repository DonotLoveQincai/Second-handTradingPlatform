#include "data/user_dao.h"

#include <QCryptographicHash>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include "data/db_manager.h"

namespace {

QString sha256Hex(const QString& s)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(s.toUtf8(), QCryptographicHash::Sha256).toHex());
}

User readRow(const QSqlQuery& q)
{
    User u;
    u.id           = q.value("id").toInt();
    u.username     = q.value("username").toString();
    u.nickname     = q.value("nickname").toString();
    u.passwordHash = q.value("password_hash").toString();
    u.salt         = q.value("salt").toString();
    u.createdAt    = q.value("created_at").toString();
    return u;
}

const char* kSelectColumns =
    "SELECT id, username, nickname, password_hash, salt, created_at FROM users ";

} // namespace

// 传进来的 passwordHash 是明文，出去一律是哈希
OpResult UserDao::insert(User& u)
{
    if (u.username.isEmpty())
        return OpResult::fail("用户名不能为空");
    if (u.passwordHash.isEmpty())
        return OpResult::fail("密码不能为空");

    if (u.salt.isEmpty())
        u.salt = QUuid::createUuid().toString(QUuid::Id128);

    const QString hash = sha256Hex(u.salt + u.passwordHash);

    QSqlQuery q(DbManager::db());
    q.prepare("INSERT INTO users(username, password_hash, salt, nickname) VALUES(?, ?, ?, ?)");
    q.addBindValue(u.username);
    q.addBindValue(hash);
    q.addBindValue(u.salt);
    q.addBindValue(u.nickname.isEmpty() ? u.username : u.nickname);

    if (!q.exec()) {
        // username 上有 UNIQUE，重复注册会走到这里
        if (q.lastError().nativeErrorCode() == "19")
            return OpResult::fail("用户名已被占用");
        return OpResult::fail("注册失败：" + q.lastError().text());
    }

    u.id = q.lastInsertId().toInt();
    u.passwordHash = hash;
    return OpResult::success(QString::number(u.id));
}

std::optional<User> UserDao::findByUsername(const QString& name)
{
    QSqlQuery q(DbManager::db());
    q.prepare(QString(kSelectColumns) + "WHERE username = ?");
    q.addBindValue(name);
    if (!q.exec() || !q.next())
        return std::nullopt;
    return readRow(q);
}

std::optional<User> UserDao::findById(int id)
{
    QSqlQuery q(DbManager::db());
    q.prepare(QString(kSelectColumns) + "WHERE id = ?");
    q.addBindValue(id);
    if (!q.exec() || !q.next())
        return std::nullopt;
    return readRow(q);
}

bool UserDao::exists(const QString& name)
{
    QSqlQuery q(DbManager::db());
    q.prepare("SELECT 1 FROM users WHERE username = ? LIMIT 1");
    q.addBindValue(name);
    return q.exec() && q.next();
}

bool UserDao::verifyPassword(const User& u, const QString& plain)
{
    if (u.salt.isEmpty() || u.passwordHash.isEmpty())
        return false;
    return sha256Hex(u.salt + plain) == u.passwordHash;
}
