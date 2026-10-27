#pragma once

#include <QString>

struct User
{
    int     id = 0;
    QString username;
    QString nickname;
    QString passwordHash;   // 传明文，DAO 内部哈希后再存
    QString salt;
    QString createdAt;
};
