#include "header.h"

class Message {
public:
    int type; // 0 代表群聊消息, 1 代表私聊消息
    std::string sendUsername; // 发送消息的用户的用户名
    std::string recvUsername; // 接收这条消息的用户的用户名, type = 0 时 recvUsername = "ALL"
    std::string content; // 消息内容
    std::chrono::system_clock::time_point time; // 发送该消息的时间

    Message();
    Message(int type, std::string sendUsername, std::string recvUsername, std::string content, std::chrono::system_clock::time_point time);

    static bool handleMessage(int client_fd, char* c_buffer);
};