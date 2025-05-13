#pragma once
#include <memory>
#include <unordered_map>
#include <mutex>
#include "Logger.hpp"

namespace log
{
    class LoggerManager
    {
    public:
        // 获取单例
        static LoggerManager &GetInstance()
        {
            static LoggerManager single;
            return single;
        }

        // 判断日志器是否存在
        bool Exist(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            auto it = map_.find(name);
            if (it == map_.end())
                return false;
            return true;
        }

        // 添加一个日志器
        void AddLogger(const Logger::ptr &&newlogger)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (Exist(newlogger->LoggerName()))
                return;
            map_.insert(std::make_pair(newlogger->LoggerName(), newlogger));
        }

        // 获取一个Logger
        Logger::ptr GetLogger(const std::string &logger_name)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            auto it = map_.find(logger_name);
            if (it == map_.end())
            {
                return default_logger_;
            }
            return it->second;
        }

    private:
        // 构造函数生成一个默认的Logger
        LoggerManager()
        {
            std::unique_ptr<LoggerBuilder> build(new LoggerBuilder());
            build->SetLoggerName("default");
            default_logger_ = build->Build();
            map_.insert(std::make_pair("default", default_logger_));
        }

    private:
        std::mutex mutex_;
        std::unordered_map<std::string, Logger::ptr> map_;
        Logger::ptr default_logger_;
    }; // end of LoggerManager

} // end of log