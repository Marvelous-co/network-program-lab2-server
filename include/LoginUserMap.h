#ifndef LOGIN_USER_MAP_H
#define LOGIN_USER_MAP_H

#include "header.h"
#include "User.h"

class LoginUserMap {
public:
    static std::unordered_map<int, User> userMap;

    static bool addLoginUser(int fd, std::string username);
    static bool removeLoginUser(int fd);
};

#endif