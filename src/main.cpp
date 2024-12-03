#include "header.h"
#include "User.h"
#include "LoginUserMap.h"

const int MAX_EVENTS = 10;
const int BUFFER_SIZE = 1024;
const int PORT = 12345;

int server_fd, epoll_fd;

// 设置文件描述符为非阻塞模式
void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
void handleSignal(int signal) {
    if (signal == SIGINT) { // 按下 Ctrl-C 时关闭套接字 server_fd, epoll_fd
        std::cout << "\nSIGINT (Ctrl-C) received. Cleaning up and exiting..." << std::endl;
        close(server_fd);
        close(epoll_fd);
        exit(0);
    }
}
int main() {
    std::signal(SIGINT, handleSignal);

    struct sockaddr_in server_addr;

    // 创建服务器套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return -1;
    }

    // 设置服务器地址和端口
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 设置套接字选项避免地址使用错误
    int on = 1;
    if ((setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0) { 
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    // 绑定服务器套接字
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_fd);
        return -1;
    }

    // 监听
    if (listen(server_fd, 10) == -1) {
        perror("Listen failed");
        close(server_fd);
        return -1;
    }

    // 设置服务器套接字为非阻塞模式
    setNonBlocking(server_fd);

    // 创建 epoll 实例
    if ((epoll_fd = epoll_create1(0)) == -1) {
        perror("Epoll creation failed");
        close(server_fd);
        return -1;
    }

    // 添加服务器套接字到 epoll
    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN; // 监听读事件
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("Epoll control failed");
        close(server_fd);
        close(epoll_fd);
        return -1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (event_count == -1) {
            perror("Epoll wait failed");
            break;
        }

        for (int i = 0; i < event_count; i++) {
            if (events[i].data.fd == server_fd) {
                // 处理新的客户端连接
                int client_fd;
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);

                while ((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len)) != -1) {
                    std::cout << "New client connected: FD " << client_fd << std::endl;
                    setNonBlocking(client_fd);

                    // 将新客户端添加到 epoll
                    event.events = EPOLLIN | EPOLLET; // 边缘触发
                    event.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                        perror("Epoll control failed for client");
                        close(client_fd);
                    }
                }
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("Accept failed");
                }
            } else {
                // 处理客户端数据
                int client_fd = events[i].data.fd;
                char buffer[BUFFER_SIZE];
                memset(buffer, 0, BUFFER_SIZE);

                int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
                if (bytes_read > 0) {
                    std::cout << "Received from FD " << client_fd << ": \n" << buffer << std::endl;
                    // 处理发送来的消息
                    if (!Message::handleMessage(client_fd, buffer)) {
                        std::cout << "main(): handleMessage ERROR!!!" << std::endl;
                    }
                } else if (bytes_read == 0) {
                    // 客户端关闭连接
                    std::cout << "Client disconnected: FD " << client_fd << std::endl;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
                    close(client_fd);
                } else {
                    perror("Read failed");
                }
            }
        }
    }

    close(server_fd);
    close(epoll_fd);
    return 0;
}