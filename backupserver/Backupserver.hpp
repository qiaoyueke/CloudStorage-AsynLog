#pragma once
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>
#include <unistd.h>

const int backlog = 32;

class Server;

class ClientData
{
public:
    ClientData(int sock, std::string &ip, uint16_t port, Server *server)
        : sock_(sock), client_ip(ip), client_port(port), server_(server)
    {
    }

    int sock_;
    std::string client_ip;
    uint16_t client_port;
    Server *server_;
};

class Server
{
public:
    using func = std::function<void(const std::string &)>;
    Server(int port, func cb) : port_(port), callback_(cb)
    {
    }

    void server_init()
    {
        listen_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == listen_socket_)
        {
            std::cout << __FILE__ << __LINE__ << "error: create socket filed" << std::endl;
            return;
        }

        struct sockaddr_in any;
        memset(&any, 0, sizeof(any));
        any.sin_family = AF_INET;
        any.sin_port = htons(port_);
        any.sin_addr.s_addr = htonl(INADDR_ANY);

        if (-1 == bind(listen_socket_, (struct sockaddr *)&any, sizeof(any)))
        {
            std::cout << __FILE__ << __LINE__ << "error: bind socket filed" << std::endl;
            return;
        }

        if (-1 == listen(listen_socket_, backlog))
        {
            std::cout << __FILE__ << __LINE__ << "error: listen socket filed" << std::endl;
            return;
        }
    }

    static void thread_work(std::shared_ptr<ClientData> cd)
    {
        std::string client_info = cd->client_ip + ":" + std::to_string(cd->client_port);
        cd->server_->service(cd->sock_, move(client_info));
        close(cd->sock_);
    }

    void server_start()
    {
        while (true)
        {
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(client_addr));
            socklen_t len = sizeof(client_addr);
            int connfd = accept(listen_socket_, (struct sockaddr *)&client_addr, &len);
            if (connfd < 0)
            {
                std::cout << __FILE__ << __LINE__ << "error: accept socket filed" << std::endl;
                continue;
            }

            uint16_t client_port = ntohs(client_addr.sin_port);
            std::string client_ip = inet_ntoa(client_addr.sin_addr);

            std::cout << __FILE__ << __LINE__ << "accept socket success" << std::endl;

            auto cd = std::make_shared<ClientData>(connfd, client_ip, client_port, this);
            std::thread t(thread_work, cd);
            t.detach();
        }
    }

    void service(int sock, const std::string &&client_info)
    {
        std::cout << __FILE__ << __LINE__ << "service start" << std::endl;

        char buf[1024];

        int r_ret = read(sock, buf, sizeof(buf));
        if (r_ret == -1)
        {
            std::cout << __FILE__ << __LINE__ << "read error" << strerror(errno) << std::endl;
            perror("NULL");
        }
        else if (r_ret > 0 && r_ret < 1024)
        {
            buf[r_ret] = 0;
            std::string tmp = buf;
            callback_(client_info + tmp); // 进行回调
        }
    }

    ~Server() = default;

private:
    int listen_socket_;
    int port_;
    func callback_;
};