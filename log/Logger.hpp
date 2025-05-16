#pragma once
#include <memory>
#include <string>
#include <vector>
#include <cstdarg>
#include <mutex>

#include "AsynscWorker.hpp"
#include "LogLevel.hpp"
#include "Message.hpp"
#include "Flush.hpp"

namespace mylog
{
    class Logger
    {
    public:
        using ptr = std::shared_ptr<Logger>;
        Logger(const std::string &name, std::vector<LogFlush::ptr> &flushs, AsyncType type)
            : logger_name_(name), flushs_(flushs.begin(), flushs.end()), type_(type),
              worker_(std::make_shared<AsynscWorker>(type_,
                                                     std::bind(&Logger::RealFlush, this, std::placeholders::_1)))
        {
            std::cout << "Logger construct start" << std::endl;
        }

        // 返回日志器的名字
        std::string LoggerName() { return logger_name_; }

        // 生成不同等级的日志
        void Debug(const std::string &filename, size_t line, const std::string format, ...)
        {
            va_list va;
            va_start(va, format);
            char *ret;
            int r = vasprintf(&ret, format.c_str(), va);
            if (r == -1)
                perror("vasprintf failed!!!: ");
            va_end(va);
            PushLog(LogLevel::value::DEBUG, filename, line, ret);
            free(ret);
            ret = nullptr;
        }

        void Info(const std::string &filename, size_t line, const std::string &format, ...)
        {
            std::cout << "Info start" << std::endl;
            va_list va;
            va_start(va, format);
            char *ret;
            int r = vasprintf(&ret, format.c_str(), va);
            if (r == -1)
                perror("vasprintf failed!!!: ");
            va_end(va);
            PushLog(LogLevel::value::INFO, filename, line, ret);
            free(ret);
            ret = nullptr;
        }

        void Warn(const std::string &filename, size_t line, const std::string &format, ...)
        {
            va_list va;
            va_start(va, format);
            char *ret;
            int r = vasprintf(&ret, format.c_str(), va);
            if (r == -1)
                perror("vasprintf failed!!!: ");
            va_end(va);
            PushLog(LogLevel::value::WARN, filename, line, ret);
            free(ret);
            ret = nullptr;
        }

        void Error(const std::string &filename, size_t line, const std::string &format, ...)
        {
            va_list va;
            va_start(va, format);
            char *ret;
            int r = vasprintf(&ret, format.c_str(), va);
            if (r == -1)
                perror("vasprintf failed!!!: ");
            va_end(va);
            PushLog(LogLevel::value::ERROR, filename, line, ret);
            free(ret);
            ret = nullptr;
        }

        void Fatal(const std::string &filename, size_t line, const std::string &format, ...)
        {
            va_list va;
            va_start(va, format);
            char *ret;
            int r = vasprintf(&ret, format.c_str(), va);
            if (r == -1)
                perror("vasprintf failed!!!: ");
            va_end(va);
            PushLog(LogLevel::value::FATAL, filename, line, ret);
            free(ret);
            ret = nullptr;
        }

    protected:
        friend AsynscWorker;

        // 根据日志等级，调用 AsynscWorker将日志内容写在生产者缓冲区
        void PushLog(const LogLevel::value level, const std::string &file, size_t line, char *ret)
        {
            std::cout << "PushLog start" << std::endl;

            Message msg(level, file, line, logger_name_, ret);

            // ERRER和FATAL有向远端存储的部分
            if (level == LogLevel::value::ERROR || level == LogLevel::value::FATAL)
            {
                // 远端存储
            }

            std::string temp = msg.format();
            std::cout <<temp<< std::endl;
            worker_->Push(temp.c_str(), temp.size());
        }

        // 交给AsynscWorker作为回调函数使用，实际使用Flushs将缓冲区中的日志写入对应文件
        void RealFlush(AsynscBuffer &buff_consumer)
        {
            if (flushs_.empty())
                return;
            for (auto a : flushs_)
            {
                a->Flush(buff_consumer.Begin(), buff_consumer.ReadableSize());
            }
        }

    private:
        AsyncType type_;
        std::mutex mutex_;
        std::string logger_name_;
        std::vector<LogFlush::ptr> flushs_;
        AsynscWorker::ptr worker_;
    };

    class LoggerBuilder
    {
    public:
        using ptr = std::shared_ptr<LoggerBuilder>;
        LoggerBuilder() = default;

        // 设置Logger名字
        void SetLoggerName(const std::string name)
        {
            logger_name_ = name;
        }

        // 设置日志器输出日志模式
        template <class LogFlushType, class... Args>
        void AddFlush(Args &&...args)
        {
            flushs_.emplace_back(std::shared_ptr<mylog::LogFlush>(std::make_shared<LogFlushType>(std::forward<Args>(args)...)));
        }

        Logger::ptr Build()
        {
            std::cout << "Build start" << std::endl;
            return std::make_shared<Logger>(logger_name_, flushs_, async_type);
        }

    private:
        std::string logger_name_;                     // 日志器名字
        std::vector<LogFlush::ptr> flushs_;           // 写日志的方式
        AsyncType async_type = AsyncType::ASYNC_SAFE; //  控制缓冲区是否增长
    }; // end of LoggerBuilder
} // end of log