#pragma once

#include <string>
#include <cstddef>
#include <string>
#include <cstdint>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sys/stat.h>
#include <fstream>
#include <iterator>


namespace mylog
{
    namespace Util
    {
        class Date
        {
        public:
            static time_t Now() { return time(nullptr); }
        };

        
        class File
        {
        public:
            bool GetFileData(std::string *content, const std::string filename)
            {
                std::ifstream ifs;
                ifs.open(filename.c_str(), std::ios::binary);
                if (ifs.is_open() == false)
                {
                    std::cout << "file open error" << std::endl;
                    return false;
                }
                *content = std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                return true;
            }

            static bool Exists(const std::string &filename)
            {
                struct stat st;
                if (0 != stat(filename.c_str(), &st))
                    return false;
                return S_ISDIR(st.st_mode);
            }

            static std::string Path(const std::string &filename)
            {
                if (filename.empty())
                    return "";
                int pos = filename.find_last_of("/\\");
                if (pos != std::string::npos)
                    return filename.substr(0, pos + 1);
                return "";
            }

            static void CreateDirectory(const std::string &pathname)
            {
                if (pathname.empty())
                    perror("文件所给路径为空：");
                // 文件不存在再创建
                if (!Exists(pathname))
                {
                    size_t pos, index = 0;
                    size_t size = pathname.size();
                    while (index < size)
                    {
                        pos = pathname.find_first_of("/\\", index);
                        if (pos == std::string::npos)
                        {
                            mkdir(pathname.c_str(), 0755);
                            return;
                        }
                        if (pos == index)
                        {
                            index = pos + 1;
                            continue;
                        }

                        std::string sub_path = pathname.substr(0, pos);
                        if (sub_path == "." || sub_path == "..")
                        {
                            index = pos + 1;
                            continue;
                        }
                        if (Exists(sub_path))
                        {
                            index = pos + 1;
                            continue;
                        }

                        mkdir(sub_path.c_str(), 0755);
                        index = pos + 1;
                    }
                }
            }
        };

        class JsonData
        {
        public:
            static JsonData *GetJsonData()
            {
                static JsonData *json_data = new JsonData;
                return json_data;
            }

        private:
            JsonData()
            {
                std::string content;
                mylog::Util::File file;
                if (file.GetFileData(&content, "Config.conf") == false)
                {
                    std::cout << __FILE__ << __LINE__ << "open config.conf failed" << std::endl;
                    perror(NULL);
                }
                nlohmann::json j = nlohmann::json::parse(content);
                buffer_size_ = j["buffer_size_"];
                threshould_ = j["threshould_"];
                line_growth_ = j["line_growth_"];
                flush_log_ = j["flush_log_"];
                backup_addr_ = j["backup_addr_"];
                backup_port_ = j["backup_port_"];
                thread_count_ = j["thread_count_"];
            }

        public:
            size_t buffer_size_;      // 缓冲区基础容量
            size_t threshould_;       // 缓冲区容量倍数扩容阈值
            size_t line_growth_;      // 缓冲区线性增长速度
            size_t flush_log_;        // 日志刷盘时机
            std::string backup_addr_; // 远程备份地址
            uint16_t backup_port_;    // 远程备份端口号
            size_t thread_count_;     // 线程池线程数
        };

    }
}