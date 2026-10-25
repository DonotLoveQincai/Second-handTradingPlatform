#pragma once

#include <optional>

#include <QString>

#include "core/common/opresult.h"
#include "core/entity/user.h"

class IUserRepository
{
public:
    virtual ~IUserRepository() = default;

    // 传明文密码，实现里加盐哈希；成功后回填 u.id
    virtual OpResult insert(User& u) = 0;

    virtual std::optional<User> findByUsername(const QString& name) = 0;
    virtual std::optional<User> findById(int id) = 0;
    virtual bool exists(const QString& name) = 0;
};
