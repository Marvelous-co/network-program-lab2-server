#ifndef USER_H
#define USER_H

#include <string>
#include <vector>
#include <utility>
#include "Message.h"

class User {
private:
    std::string name;
    // 一个用户发送的消息列表
    // 每条消息由一个 pair 构成, int: 0 代表群聊消息 1 代表私聊消息, string: 消息内容
    std::vector<Message*> message;

public:
    // 构造函数
    User() : name("default") {} // 默认构造函数
    User(const std::string& name);

    void setName(const std::string& name);
    std::string getName() const;

    void pushMessage(Message* msg);
    std::vector<Message*> getMessageArr() const;

    // 生成随机的用户名
    static std::string generateRandomUsername();
};

#endif // USER_H