#include "LoginUserMap.h"
#include "User.h"
#include "Message.h"


const std::string SEND_INDIVIDUALLY_FORMAT = "[SEND INDIVIDUALLY]";
const std::string SEND_TO_ALL_FORMAT = "[SEND TO ALL]";
std::string split(std::string str, int& i, int& j, bool flag);
/**
 * @description: 处理私发消息
 * @param: client_fd: 发送消息人的文件描述符, username: 接收消息的用户名, msg: 消息内容
 * @return: 发送是否成功
 */
bool handleIndividualMsg(int client_fd, std::string username, std::string msg) {
    auto map = LoginUserMap::userMap;

    for (auto it = map.begin(); it != map.end(); it++) {
        if (it->second.getName() == username) { // 找到了要转发的收信人
            int sendfd = it->first;
            if (map.find(client_fd) == map.end()) {
                std::cout << "don't find client_fd : " << client_fd <<  " in userMap";
                return false;
            }
            std::string recvUsername = map[client_fd].getName();

            std::string msgIncludeHeader = SEND_INDIVIDUALLY_FORMAT + "\n" +
                                        "[" + recvUsername + "]" + "\n"
                                        + msg;
            if (write(sendfd, msgIncludeHeader.c_str(), msgIncludeHeader.size()) == -1) {
                std::cout << "write msg to username: " + username << " error" << std::endl;
                return false;
            } else {
                std::cout << "write msg to username: " + username << " successfully" << std::endl;
                return true;
            }
        }
    }
    std::cout << "don't find fd map from username: " + username << std::endl;
    return false;
}

/**
 * @description: 处理群发消息
 * @param: client_fd: 发送消息人的文件描述符, msg: 消息内容
 * @return: 群发是否成功
 */
bool handleAllMsg(int client_fd, std::string msg) {
    auto map = LoginUserMap::userMap;
    for (auto it = map.begin(); it != map.end(); it++) {
        int fd = it->first;
        if (fd != client_fd) {
            std::string msgIncludeHeader = SEND_TO_ALL_FORMAT + "\n" + 
                                            "[ALL]" + "\n" + 
                                            msg;
            if (write(fd, msgIncludeHeader.c_str(), msgIncludeHeader.size()) == -1) {
                std::cout << "write msg to ALL USER error" << std::endl;
                return false;
            }
        }
    }
    std::cout << "write msg to ALL USER successfully" << std::endl;
    return true;
}
/**
 * buffer 格式:
 * [SEND INDIVIDUALLY / SEND TO ALL]
 * [收件人的用户名/ALL]
 * 发送消息内容
 * 
 * e.g. :
 * [SEND INDIVIDUALLY]
 * [ccr]
 * hello, world, this message is send to ccr
 * 
 * @description: 针对 buffer 的内容不同对消息进行不同的处理：
 *               1. 如果是群发消息，则将消息转发给 Map 中所有的 fd
 *               2. 如果是私发消息，则将消息转发给 Map 中对应的 fd
 * @param: client_fd: 发件人的文件描述符 client_fd, c_buffer: 发送信息
 * @return: bool: 处理消息是否成功
 */
bool Message::handleMessage(int client_fd, char* c_buffer) {
    std::string buffer = c_buffer;

    int i = 0, j = 0;
    std::string header = split(buffer, i, j, false);
    if (header == "") {
        std::cout << "recv msg don't contain header" << std::endl;
        return false;
    }
    std::string username = split(buffer, i, j, false);
    if (username == "") {
        std::cout << "recv msg don't contain username" << std::endl;
        return false;
    }
    std::string msg = split(buffer, i, j, true);
    std::cout << "header: " << header << "\nusername: " + username + "\nmsg: " + msg << std::endl;
    if (header == SEND_INDIVIDUALLY_FORMAT) {
        return handleIndividualMsg(client_fd, username, msg);
    } else if (header == SEND_TO_ALL_FORMAT && username == "[ALL]") {
        return handleAllMsg(client_fd, msg);
    }
    return false;
}

std::string split(std::string str, int& i, int& j, bool flag) {
    while (j < str.size() && str[j] != '\n')
        j++;
    if (j >= str.size() && !flag)
        return "";
    auto ret = str.substr(i, j - i);
    j = j + 1;
    i = j;
    return ret;
}