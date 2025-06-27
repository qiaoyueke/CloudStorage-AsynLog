#pragma once
#include <string>
#include <cstdint>
#include <sys/stat.h>
#include <fstream>
#include <experimental/filesystem>
#include "../log/MyLog.hpp"
#include "bundle.h"
namespace storage
{
    unsigned char FromHex(unsigned char x)
    {
        unsigned char y;
        if (x >= 'A' && x <= 'Z')
            y = x - 'A' + 10;
        else if (x >= 'a' && x <= 'z')
            y = x - 'a' + 10;
        else if (x >= '0' && x <= '9')
            y = x - '0';
        else
            assert(0);
        return y;
    }

    std::string UrlDecode(const std::string &str)
    {
        std::string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            // if (str[i] == '+')
            //     strTemp += ' ';
            if (str[i] == '%')
            {
                assert(i + 2 < length);
                unsigned char high = FromHex((unsigned char)str[++i]);
                unsigned char low = FromHex((unsigned char)str[++i]);
                strTemp += high * 16 + low;
            }
            else
                strTemp += str[i];
        }
        return strTemp;
    }

    class FileUtil
    {
    private:
        std::string filename_;

    public:
        FileUtil(const std::string &filename)
            : filename_(filename) {}

        // 获取文件大小
        int64_t FileSize()
        {
            struct stat s;
            auto fs = stat(filename_.c_str(), &s);
            if (fs == -1)
            {
                mylog::GetLogger()->Info("%s, Get file size failed: %s", filename_.c_str(), strerror(errno));
                return -1;
            }
            return s.st_size;
        }

        // 获取最近访问时间
        time_t LastAccessTime()
        {
            struct stat s;
            auto ret = stat(filename_.c_str(), &s);
            if (ret == -1)
            {
                mylog::GetLogger()->Info("%s, Get file access time failed: %s", filename_.c_str(), strerror(errno));
                return -1;
            }
            return s.st_atime;
        }

        // 获取最近修改时间
        time_t LastChangeTime()
        {
            struct stat s;
            auto ret = stat(filename_.c_str(), &s);
            if (ret == -1)
            {
                mylog::GetLogger()->Info("%s, Get file change time failed: %s", filename_.c_str(), strerror(errno));
                return -1;
            }
            return s.st_mtime;
        }

        // 获取文件名
        std::string FileName()
        {
            auto pos = filename_.find_last_of("/");
            if (pos == std::string::npos)
                return filename_;
            return filename_.substr(pos + 1, std::string::npos);
        }

        // 获取从pos开始，len长度的内容，放在cotent所指字符串里
        bool get_pos_len(std::string *content, size_t pos, size_t len)
        {
            if (pos + len > FileSize())
            {
                mylog::GetLogger()->Info("needed data larger than file size");
                return false;
            }

            // 打开文件
            std::ifstream ifs;
            ifs.open(filename_.c_str(), std::ios::binary);
            if (ifs.is_open() == false)
            {
                mylog::GetLogger()->Info("%s,file open error", filename_.c_str());
                return false;
            }

            // 读入content
            ifs.seekg(pos, std::ios::beg); // 更改文件指针的偏移量
            content->resize(len);
            ifs.read(&(*content)[0], len);
            if (!ifs.good())
            {
                mylog::GetLogger()->Info("%s,read file content error", filename_.c_str());
                ifs.close();
                return false;
            }
            ifs.close();

            return true;
        }

        // 获取完整文件内容
        bool get_file(std::string *content)
        {
            return get_pos_len(content, 0, FileSize());
        }

        // content开始，长度为len的字符写入文件（覆盖原有内容）
        bool write_file(const char *content, const size_t len)
        {
            std::ofstream ofs;
            ofs.open(filename_.c_str(), std::ios::binary);
            if (!ofs.is_open())
            {
                mylog::GetLogger()->Info("%s open error: %s", filename_.c_str(), strerror(errno));
                return false;
            }
            ofs.write(content, len);
            if (!ofs.good())
            {
                mylog::GetLogger()->Info("%s, file set content error", filename_.c_str());
                ofs.close();
            }
            ofs.close();
            return true;
        }

        bool Exists()
        {
            struct stat st;
            if (0 != stat(filename_.c_str(), &st))
                return false;
            return !S_ISDIR(st.st_mode);
        }

        void creat_dir()
        {
            if (std::experimental::filesystem::exists(filename_))
            {
                return;
            }
            std::experimental::filesystem::create_directory(filename_);
        }

        bool compress(std::string &content, int format)
        {
            content = bundle::pack(format, content);
            if(0 == content.size())
            {
                mylog::GetLogger()->Error("compress error");
                return false;
            }
            return true;
        }

        bool uncompress(std::string &path)
        {
            // 将当前压缩包数据读取出来
            std::string body;
            if (this->get_file(&body) == false)
            {
                mylog::GetLogger()->Info("filename:%s, uncompress get file content failed!",filename_.c_str());
                return false;
            }
            // 对压缩的数据进行解压缩
            std::string unpacked = bundle::unpack(body);
            // 将解压缩的数据写入到新文件
            FileUtil fu(path);
            if (fu.write_file(unpacked.c_str(), unpacked.size()) == false)
            {
                mylog::GetLogger()->Info("filename:%s, uncompress write packed data failed!",filename_.c_str());
                return false;
            }
            return true;
        }
    };

}