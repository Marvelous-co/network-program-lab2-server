#include "LoginUserMap.h"
#include <iomanip>
#include <ctime>
#include <sstream>

const std::string SEND_INDIVIDUALLY_FORMAT = "[SEND INDIVIDUALLY]";
const std::string SEND_TO_ALL_FORMAT = "[SEND TO ALL]";
const std::string LOGIN = "[LOGIN]";
const std::string LOGOUT = "[LOGOUT]";
const std::string GET_LOGINUSER = "[GET LOGINUSER]";
const std::string HISTORY_MESSAGE = "[HISTORY MESSAGE]";

Message::Message() {}
Message::Message(int type, std::string sendUsername, std::string recvUsername, std::string content, std::chrono::system_clock::time_point time) {
    this->type = type;
    this->sendUsername = sendUsername;
    this->recvUsername = recvUsername;
    this->content = content;
    this->time = time;
}
std::string split(std::string str, int& i, int& j, bool flag);

/**
 * @description: 根据传入的 header, username, body 构造 message
 * @param: 消息的三个构成部分，之间以换行符形式分隔, 其中 header 默认包含 [], username 默认不包含 []
 */
std::string buildMsg(std::string header, std::string username, std::string body) {
    return header + "\n[" + username + "]\n" + body + "\n";
} 
/**
 * @description: 在构造消息时使用, 将 Message 中的各个字段拼成一个字符串, 之间以 \0x01(不可见字符) 分隔
 * @param: Message 类中的各个属性
 */
std::string buildContent(int type, std::string sendUsername, std::string recvUsername, 
                        std::string msgBody, std::string timestampStr) {
    std::time_t timestamp = std::stoll(timestampStr);
    std::chrono::system_clock::time_point timePoint = std::chrono::system_clock::from_time_t(timestamp);
    // 转换为 time_t
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    // 格式化时间为字符串
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    std::string formatTime = oss.str();

    return std::to_string(type) + "\x01" + sendUsername + "\x01" + recvUsername + "\x01"  + msgBody + "\x01" + formatTime;
}
std::string buildContent(Message m) {
    std::time_t timeT = std::chrono::system_clock::to_time_t(m.time);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
    std::string formattedTime = oss.str();

    return std::to_string(m.type) + "\x01" + m.sendUsername +  "\x01" + m.recvUsername +  "\x01" + m.content +  "\x01" + formattedTime;
}
/**
 * @description: 将消息存入对应 User 类中的 message 数组中
 * @param: client_fd: 发送用户对应的文件描述符
 *         msgBody: 消息内容
 *         type: 消息类型, 0 代表群聊消息, 1 代表私聊消息
 *         
 */
void storeMsg(int client_fd, int type, const std::string& recvName, const std::string& content, const std::string& timestamp) {
    std::string sendName = LoginUserMap::userMap[client_fd].getName();
    std::chrono::system_clock::time_point time = std::chrono::system_clock::from_time_t(std::stoll(timestamp));

    LoginUserMap::userMap[client_fd].pushMessage(new Message(type, sendName, recvName, content, time));
}
/**
 * @description: 当客户端发来消息时，消息体由 "time msg"组成，该函数将 body 分割成 msgBody 和 time
 * @param
 * @return: 分割是否成功
 */
bool splitBody(const std::string& body, std::string& msgBody, std::string& time) {
    int i = 0, j = 0;
    while (j < body.size() && body[j] != ' ')
        j++;
    if (j == body.size() || j == body.size() - 1)
        return false;
    time = body.substr(0, j);
    msgBody = body.substr(j + 1, body.size() - j - 1);
    return true;
}
/**
 * @description: 处理私发消息
 * @param: client_fd: 发送消息人的文件描述符, username: 接收消息的用户名, body: 时间 + 具体消息
 * @return: 发送是否成功
 */
bool handleIndividualMsg(int client_fd, std::string username, std::string body) {
    std::string msgBody, timestamp;
    if (!splitBody(body, msgBody, timestamp)) {
        std::cout << "split body error, body: " << body << std::endl;
        return false;
    }
    // 存入私聊消息
    storeMsg(client_fd, 1, username, msgBody, timestamp);
    
    auto map = LoginUserMap::userMap;
    for (auto it = map.begin(); it != map.end(); it++) {
        if (it->second.getName() == username) { // 找到了要转发的收信人
            int sendfd = it->first;
            if (map.find(client_fd) == map.end()) {
                std::cout << "don't find client_fd : " << client_fd <<  " in userMap";
                return false;
            }            
            std::string sendUsername = map[client_fd].getName();
            std::string recvUsername = username;

            std::string content = buildContent(1, sendUsername, recvUsername, msgBody, timestamp);
            std::string msg = buildMsg(SEND_INDIVIDUALLY_FORMAT, recvUsername, content);
            if (write(sendfd, msg.c_str(), msg.size()) == -1) {
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
 * @param: client_fd: 发送消息人的文件描述符, body: 消息内容
 * @return: 群发是否成功
 */
bool handleAllMsg(int client_fd, std::string body) {
    std::string msgBody, timestamp;
    if (!splitBody(body, msgBody, timestamp)) {
        std::cout << "split body error, body: " << body << std::endl;
        return false;
    }
    // 存入群聊消息
    storeMsg(client_fd, 0, "ALL", msgBody, timestamp);
    auto map = LoginUserMap::userMap;
    for (auto it = map.begin(); it != map.end(); it++) {
        int fd = it->first;
        if (fd != client_fd) {
            int type = 0;
            std::string sendUsername = LoginUserMap::userMap[client_fd].getName();
            std::string recvUsername = "ALL";
            std::string content = buildContent(type, sendUsername, recvUsername, msgBody, timestamp);

            std::string msg = buildMsg(SEND_TO_ALL_FORMAT, sendUsername, content);
            if (write(fd, msg.c_str(), msg.size()) == -1) {
                std::cout << "write msg to ALL USER error" << std::endl;
                return false;
            }
        }
    }
    std::cout << "write msg to ALL USER successfully" << std::endl;
    return true;
}

/**
 * @description: 将 LOGIN/LOGOUT 消息转发给除了发送来的 client 的其他所有的 client
 * @param: client_fd: 发送 LOGIN/LOGOUT 的客户端, msg: 需要转发的消息，已包含 [header] 和 [username]
 */
bool transferMsg(int client_fd, std::string msg) {
    auto map = LoginUserMap::userMap;
    for (auto it = map.begin(); it != map.end(); it++) {
        int fd = it->first;
        if (fd != client_fd) {
            if (write(fd, msg.c_str(), msg.size()) == -1) {
                std::cout << "write LOGIN/LOGOUT MSG to ALL USER error" << std::endl;
                return false;
            } else {
                std::cout << "write LOGIN/LOGOUT MSG to user: " << map[fd].getName() << std::endl;
            }
        }
    }
    return true;
}

/**
 * @description: 在用户登录系统时回送消息，body 中携带当前系统已经登录用户的用户名信息
 *              格式为: [GET LOGINUSER]
 *                     [username] // 登录用户的 username
 *                     ccr1 ccr2 ccr3 ... // 已经登陆系统的用户名，以 space ' ' 分隔
 */
void retLoginUserMsg(int client_fd, std::string username) {
    // 构造 body 中的内容
    std::string body = "";
    auto map = LoginUserMap::userMap;
    for (auto it = map.begin(); it != map.end(); it++) {
        body += it->second.getName() + " ";
    }
    std::string msg = buildMsg(GET_LOGINUSER, username, body);
    if (write(client_fd, msg.c_str(), msg.size()) == -1) {
        std::cout << "write ALERADY LOGIN USER MSG to user " <<  map[client_fd].getName() << " error" << std::endl;
    } else {
        std::cout << "write ALERADY LOGIN USER MSG to user " <<  map[client_fd].getName() << std::endl;
    }
}
/**
 * @description: 处理用户登录
 * @param: client_fd: 发送消息人的文件描述符, username: 登录用户的用户名, msg: 需要转发的信息(包含 header 和 username 部分)
 * @return: 用户登录/登出信息转发是否成功
 */
bool handleLogin(int client_fd, std::string username, std::string msg) {
    // 将用户信息插入到 LoginUserMap 中
    LoginUserMap::addLoginUser(client_fd, username);
    // 向登录用户返回已经登录的用户的用户名
    retLoginUserMsg(client_fd, username); 
    // 向其他用户返回该用户已经登录
    return transferMsg(client_fd, msg);
}
bool handleLogout(int client_fd, std::string msg) {
    // 将用户信息从 LoginUserMap 中删除
    LoginUserMap::removeLoginUser(client_fd);

    return transferMsg(client_fd, msg);
}
/**
 * @description: 处理用户请求历史消息, 从内存中查询出相关信息并返回
 *               返回的每条消息的格式: 消息类型, 发送人用户名, 接收人用户名, 消息内容, 发送时间
 * @param: client_fd: 请求人的文件描述符信息, username: [ALL/对端用户名]
 * @return: 获取信息是否成功
 */
bool handleHistoryMessage(int client_fd, std::string username) {
    std::string retMsgBody;
    std::string clientName = LoginUserMap::userMap[client_fd].getName();
    auto map = LoginUserMap::userMap;
    if (username == "ALL") {
        // 从内存中遍历所有用户，并从这些用户中找出群聊消息
        for (auto it = map.begin(); it != map.end(); it++) {
            auto msgArr = it->second.getMessageArr();
            for (auto m : msgArr) {
                if (m->type == 0) { // 如果该消息属于群聊消息
                    retMsgBody += buildContent(*m) + "\x02";
                }
            }
        }
    } else { 
        for (auto it = map.begin(); it != map.end(); it++) {
            if (it->second.getName() == username) { // 找到私聊对方的用户名
                for (auto m: it->second.getMessageArr()) {
                    // 如果是私发给自己的
                    if (m->type = 1 && m->recvUsername == clientName) {
                        retMsgBody += buildContent(*m) + "\x02";
                    }
                }
            } else if (it->first == client_fd) { // 获取请求人发出的消息
                for (auto m: it->second.getMessageArr()) {
                    if (m->type == 1 && m->recvUsername == username) {
                        retMsgBody += buildContent(*m) + "\x02";
                    }
                } 
            }
        }
    }
    std::string retMsg = buildMsg(HISTORY_MESSAGE, username, retMsgBody);
    write(client_fd, retMsg.c_str(), retMsg.size());
    return true;
}
/**
 * buffer 格式:
 * [SEND INDIVIDUALLY / SEND TO ALL / LOGIN / LOGOUT / HISTORY MESSAGE]
 * [收件人的用户名 / ALL / 登录用户的用户名 / 登出用户的用户名 / [ALL, username]]
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
 *               3. 如果是有用户登录，则将 [登录] 这一消息转发给其他的用户
 *               4. 如果是有用户登出，则将 [登出] 这一消息转发给其他的用户
 *               5. 如果是查询历史消息，则 username 有 [ALL/username] 两种，分别是请求私聊消息和群聊消息
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
    // 删除头尾元素的 '[' 和 ']'
    username.erase(0, 1);
    username.erase(username.size() - 1);
    if (username == "") {
        std::cout << "recv msg don't contain username" << std::endl;
        return false;
    }
    std::string msg = split(buffer, i, j, true);
    std::cout << "header: " << header << "\nusername: " + username + "\nmsg: " + msg << std::endl;
    
    // 消息分发给相应的函数进行处理
    if (header == SEND_INDIVIDUALLY_FORMAT) {
        return handleIndividualMsg(client_fd, username, msg);
    } else if (header == SEND_TO_ALL_FORMAT && username == "ALL") {
        return handleAllMsg(client_fd, msg);
    } else if (header == LOGIN) {
        return handleLogin(client_fd, username, buffer);
    } else if (header == LOGOUT) {
        return handleLogout(client_fd, buffer);
    } else if (header == HISTORY_MESSAGE) {
        return handleHistoryMessage(client_fd, username);
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