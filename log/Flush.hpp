#pragma once

#include <memory>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "Util.hpp"

extern mylog::Util::JsonData *g_conf_data;

namespace mylog
{
    // Flush的基类
    class LogFlush
    {
    public:
        using ptr = std::shared_ptr<LogFlush>;
        virtual ~LogFlush() = default;
        virtual void Flush(const char *begin, size_t len) = 0;
    };

    class CoutFlush : public LogFlush
    {
    public:
        using ptr = std::shared_ptr<CoutFlush>;
        void Flush(const char *begin, size_t len) override
        {
            std::cout.write(begin, len);
        }
    };

    class FileFlush : public LogFlush
    {
    public:
        FileFlush(const std::string filename) : filename_(filename)
        {
            mylog::Util::File::CreateDirectory(mylog::Util::File::Path(filename_));
            fs_ = fopen(filename.c_str(), "ab");
            if (fs_ == NULL)
            {
                std::cout << __FILE__ << __LINE__ << "open log file failed" << std::endl;
                perror(NULL);
            }
        }

        void Flush(const char *begin, size_t len) override
        {
            fwrite(begin, 1, len, fs_);
            if (ferror(fs_))
            {
                std::cout << __FILE__ << __LINE__ << "write log file failed" << std::endl;
                perror(NULL);
            }
            if (g_conf_data->flush_log_ == 1)
            {
                if (fflush(fs_) == EOF)
                {
                    std::cout << __FILE__ << __LINE__ << "fflush file failed" << std::endl;
                    perror(NULL);
                }
            }
            else if (g_conf_data->flush_log_ == 2)
            {
                fflush(fs_);
                fsync(fileno(fs_));
            }
        }

    private:
        std::string filename_;
        FILE *fs_ = NULL;
    };

    class RollFileFlush : public LogFlush
    {
    public:
        using ptr = std::shared_ptr<RollFileFlush>;
        RollFileFlush(const std::string &filename, size_t max_size)
            : max_size_(max_size), basename_(filename)
        {
            Util::File::CreateDirectory(Util::File::Path(filename));
        }

        void Flush(const char *data, size_t len) override
        {
            // 确认文件大小不满足滚动需求
            InitLogFile();
            // 向文件写入内容
            fwrite(data, 1, len, fs_);
            if (ferror(fs_))
            {
                std::cout << __FILE__ << __LINE__ << "write log file failed" << std::endl;
                perror(NULL);
            }
            cur_size_ += len;
            if (g_conf_data->flush_log_ == 1)
            {
                if (fflush(fs_))
                {
                    std::cout << __FILE__ << __LINE__ << "fflush file failed" << std::endl;
                    perror(NULL);
                }
            }
            else if (g_conf_data->flush_log_ == 2)
            {
                fflush(fs_);
                fsync(fileno(fs_));
            }
        }

    private:
        void InitLogFile()
        {
            if (fs_ == NULL || cur_size_ >= max_size_)
            {
                if (fs_ != NULL)
                {
                    fclose(fs_);
                    fs_ = NULL;
                }
                std::string filename = CreateFilename();
                fs_ = fopen(filename.c_str(), "ab");
                if (fs_ == NULL)
                {
                    std::cout << __FILE__ << __LINE__ << "open file failed" << std::endl;
                    perror(NULL);
                }
                cur_size_ = 0;
            }
        }

        // 构建落地的滚动日志文件名称
        std::string CreateFilename()
        {
            time_t time_ = Util::Date::Now();
            struct tm t;
            localtime_r(&time_, &t);
            std::string filename = basename_;
            filename += std::to_string(t.tm_year + 1900);
            filename += std::to_string(t.tm_mon + 1);
            filename += std::to_string(t.tm_mday);
            filename += std::to_string(t.tm_hour + 1);
            filename += std::to_string(t.tm_min + 1);
            filename += std::to_string(t.tm_sec + 1) + '-' +
                        std::to_string(cnt_++) + ".log";
            return filename;
        }

    private:
        size_t cnt_ = 1;
        size_t cur_size_ = 0;
        size_t max_size_;
        std::string basename_;
        FILE *fs_ = NULL;
    };

}