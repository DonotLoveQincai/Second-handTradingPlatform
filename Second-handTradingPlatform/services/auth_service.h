#pragma once

#include <QObject>
#include <QString>

#include "core/common/opresult.h"

class IUserRepository;

// 注册、登录、退出。登录态存 Session，界面靠信号刷新。
class AuthService : public QObject
{
    Q_OBJECT
public:
    explicit AuthService(IUserRepository* repo, QObject* parent = nullptr);

    OpResult login(const QString& username, const QString& password);
    OpResult registerUser(const QString& username, const QString& nickname,
                          const QString& pwd, const QString& pwd2);
    void logout();

signals:
    void loginSucceeded();
    void logoutDone();

private:
    IUserRepository* m_repo;
};
