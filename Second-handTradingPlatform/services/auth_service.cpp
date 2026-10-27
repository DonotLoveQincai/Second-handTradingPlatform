#include "services/auth_service.h"

#include "core/entity/user.h"
#include "core/repository/i_user_repository.h"
#include "core/session.h"
#include "data/user_dao.h"

AuthService::AuthService(IUserRepository* repo, QObject* parent)
    : QObject(parent)
    , m_repo(repo)
{
}

OpResult AuthService::login(const QString& username, const QString& password)
{
    if (username.trimmed().isEmpty())
        return OpResult::fail(QStringLiteral("请输入用户名"));

    const auto u = m_repo->findByUsername(username.trimmed());
    if (!u || !UserDao::verifyPassword(*u, password))
        return OpResult::fail(QStringLiteral("用户名或密码错误"));   // 不说是哪个错

    Session::instance().setCurrentUser(*u);
    emit loginSucceeded();
    return OpResult::success();
}

OpResult AuthService::registerUser(const QString& username, const QString& nickname,
                                   const QString& pwd, const QString& pwd2)
{
    if (username.trimmed().size() < 3)
        return OpResult::fail(QStringLiteral("用户名至少 3 个字符"));
    if (pwd.size() < 6)
        return OpResult::fail(QStringLiteral("密码至少 6 位"));
    if (pwd != pwd2)
        return OpResult::fail(QStringLiteral("两次密码不一致"));
    if (m_repo->exists(username.trimmed()))
        return OpResult::fail(QStringLiteral("用户名已被占用"));

    User u;
    u.username = username.trimmed();
    u.nickname = nickname.trimmed().isEmpty() ? u.username : nickname.trimmed();
    u.passwordHash = pwd;   // 明文交给 DAO，它负责加盐哈希

    const OpResult r = m_repo->insert(u);
    if (!r.ok)
        return r;

    Session::instance().setCurrentUser(u);
    emit loginSucceeded();
    return OpResult::success();
}

void AuthService::logout()
{
    Session::instance().clear();
    emit logoutDone();
}
