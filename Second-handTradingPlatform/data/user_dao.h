#pragma once

#include <QString>

#include "core/repository/i_user_repository.h"

class UserDao : public IUserRepository
{
public:
    OpResult insert(User& u) override;
    std::optional<User> findByUsername(const QString& name) override;
    std::optional<User> findById(int id) override;
    bool exists(const QString& name) override;

    // 拿库里那条记录的盐重算一遍再比，不能直接比哈希
    static bool verifyPassword(const User& u, const QString& plain);
};
