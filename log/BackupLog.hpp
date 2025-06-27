#pragma

#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "Util.hpp"

extern mylog::Util::JsonData *g_conf_data;

namespace mylog
{
    void Backup(const std::string &msg)
    {
        // 创建socket链接

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
        {
            std::cout << __FILE__ << __LINE__ << "socket error : " << strerror(errno) << std::endl;
            perror(NULL);
        }

        // 配置服务端地址与接口
        struct sockaddr_in server;
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_port = htons(g_conf_data->backup_port_);
        inet_aton(g_conf_data->backup_addr_.c_str(), &server.sin_addr);

        std::cout << "尝试链接 " << std::endl;

        int cnts = 5;
        while (-1 == connect(sock, (struct sockaddr *)&server, sizeof(server)))
        {
            std::cout << "正在尝试重连,重连次数还有: " << cnts-- << std::endl;
            if (cnts <= 0)
            {
                std::cout << __FILE__ << __LINE__ << "connect error : " << strerror(errno) << std::endl;
                close(sock);
                perror(NULL);
                return;
            }
        }
        
        std::cout << "链接成功 " << std::endl;

        char buffer[1024];
        std::cout << "开始发送  " << std::string(buffer) << std::endl;
        if (-1 == write(sock, msg.c_str(), msg.size()))
        {
            std::cout << __FILE__ << __LINE__ << "send to server error : " << strerror(errno) << std::endl;
            perror(NULL);
        }
        close(sock);
    }
}