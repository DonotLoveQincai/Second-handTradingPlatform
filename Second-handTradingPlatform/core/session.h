#pragma once

#include "core/entity/user.h"

class Session
{
public:
    static Session& instance() { static Session s; return s; }

    const User& currentUser() const { return m_user; }
    bool isLoggedIn() const         { return m_user.id > 0; }
    void setCurrentUser(const User& u) { m_user = u; }
    void clear()                    { m_user = User(); }

private:
    Session() = default;

    User m_user;
};
