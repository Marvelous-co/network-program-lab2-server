#include "header.h"

class Message {
public:
    static bool handleMessage(int client_fd, char* c_buffer);
};