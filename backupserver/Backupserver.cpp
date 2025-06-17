// 远程备份debug等级以上的日志信息-接收端
#include <string>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "Backupserver.hpp"

using std::cout;
using std::endl;

const std::string basename = "./logfile";
int cnt = 0;
const long max_size = 10000000;

void usage(std::string procgress)
{
    cout << "usage error:" << procgress << "port" << endl;
}

void backup_log(const std::string &message) // 用作回调
{
    std::string filename = basename + std::to_string(cnt);
    FILE *fp = fopen(filename.c_str(), "ab");
    if (fp == NULL)
    {
        perror("fopen error: ");
        assert(false);
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    if (size > max_size)
    {
        fclose(fp);
        cnt++;
        filename = basename + std::to_string(cnt);
        fp = fopen(filename.c_str(), "ab");

        if (fp == NULL)
        {
            perror("fopen error: ");
            assert(false);
        }
    }

    int write_byte = fwrite(message.c_str(), 1, message.size(), fp);
    if (write_byte != message.size())
    {
        perror("fwrite error: ");
        assert(false);
    }

    fflush(fp);
    fclose(fp);
}


int main(int args, char *argv[])
{
    if (args != 2)
    {
        usage(argv[0]);
        perror("usage error");
        exit(-1);
    }

    uint16_t port = atoi(argv[1]);
    std::unique_ptr<Server> tcp(new Server(port, backup_log));

    tcp->server_init();
    tcp->server_start();

    return 0;
}