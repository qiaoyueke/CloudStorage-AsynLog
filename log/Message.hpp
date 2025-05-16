#pragma once 
#include<ctime>
#include<string>
#include<thread>
#include<sstream>
#include<memory>
#include"LogLevel.hpp"

namespace mylog{
    class Message
    {
        public:
        using ptr = std::shared_ptr<Message>;
        Message() = default;
        
        Message(LogLevel::value level, const std::string &name, size_t line, const std::string & loggername, char* ret)
        :level_(LogLevel::level_to_string(level)), file_name_(name), line_(line), logger_name_(loggername), data_(ret),
        tid_(std::this_thread::get_id())
        {}

        std::string format() const {
            std::stringstream ss;

            struct tm t;
            localtime_r(&time_, &t); 
            char time[128];
            strftime(time, sizeof(time), "%Y-%m-%d %H:%M:%S", &t);
            std::string s1 = "[" + std::string(time) + "][" ;
            std::string s2 = "][" + level_ + "][" + logger_name_ + "][" + file_name_ + "][" + std::to_string(line_) + "]\t" + std::string(data_) + "\n";
            ss<<s1<<tid_<<s2;
            return ss.str();
        }

        private:
        time_t time_;
        std::thread::id tid_;
        std::string logger_name_;
        std::string level_;
        std::string file_name_;
        size_t line_;
        char* data_;
    };

}