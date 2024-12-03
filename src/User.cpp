#include "User.h"
#include <chrono>
#include <random>


// 构造函数
User::User(const std::string& name) {
    this->name = name;
}

void User::setName(const std::string& name) {
    this->name = name;
}

std::string User::getName() const {
    return name;
}

void User::pushMessage(Message* msg) {
    this->message.push_back(msg);
}

std::vector<Message*> User::getMessageArr() const {
    return this->message;
}

/**
 * @description: 随机生成用户名, 格式为 "用户-当前的毫秒时间戳-[0-99]随机数字"
 * @return: std::string 生成的随机用户名
 */
std::string User::generateRandomUsername() {
    auto now = std::chrono::system_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    int64_t prefix = duration.count();

    std::random_device rd;
    std::mt19937 gen(rd());
    
    std::uniform_int_distribution<> dist(0, 99);
    // 生成 0-99 范围内的 随机整数
    int randomNumber = dist(gen);

    return "用户-" + std::to_string(prefix) + "-" + std::to_string(randomNumber);
}