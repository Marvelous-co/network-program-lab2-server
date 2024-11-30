#ifndef USER_H
#define USER_H

#include <string>
#include <vector>

class User {
private:
    std::string name;
    std::vector<std::string> message;

public:
    // 构造函数
    User() : name("default") {} // 默认构造函数
    User(const std::string& name);

    void setName(const std::string& name);
    std::string getName() const;

    void pushMessage(const std::string& message);
    std::vector<std::string> getMessageArr() const;

    // 生成随机的用户名
    static std::string generateRandomUsername();
};

#endif // USER_H