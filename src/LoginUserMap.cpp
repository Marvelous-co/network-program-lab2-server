#include "header.h"
#include "LoginUserMap.h"

std::unordered_map<int, User> LoginUserMap::userMap;
/**
 * @description: 向 LoginUserMap 中添加登录用户
 * @return: true: 添加成功, false: 添加失败, map 中已经有对应的 fd
 */
bool LoginUserMap::addLoginUser(int fd, std::string username) {
    if (LoginUserMap::userMap.find(fd) != LoginUserMap::userMap.end()) {
        return false;
    }
    User* user = new User(username);
    LoginUserMap::userMap[fd] = *user;
    return true;
}

/**
 * @description: 从 LoginUserMap 中移除用户
 * @return: true: 移除成功, false: 移除失败(原因: map 中没有传入的 fd 或移除的元素个数不为 1)
 */
bool LoginUserMap::removeLoginUser(int fd) {
    if (LoginUserMap::userMap.find(fd) == LoginUserMap::userMap.end()) {
        return false;
    }
    int cnt = LoginUserMap::userMap.erase(fd);
    if (cnt != 1) {
        return false;
    }
    return true;
}