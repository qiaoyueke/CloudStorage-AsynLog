#pragma once
#include<memory>
#include<string>
#include<vector>
#include<mutex>
#include"Flush.hpp"
#include"AsynscWorker.hpp"

namespace log{
    class Logger{
        public:
        using ptr = std::shared_ptr<Logger>;
        Logger(const std::string& name, std::vector<LogFlush>& flushs, AsyncType type)
        :logger_name_(name), flushs_(flushs.begin(), flushs.end()), type_(type),
        worker_(std::make_shared<AsynscWorker>(type_,
        std::bind(&Logger::RealFlush, this, std::placeholders::_1)))
        {}

        //返回日志器的名字
        std::string LoggerName(){
            return logger_name_;
        }


        protected:
        friend AsynscWorker;
        void RealFlush(const AsynscBuffer& buff_consumer){

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
        LoggerBuilder() = default;

        void SetLoggerName(const std::string name){
            logger_name_ = name;
        }

        Logger::ptr Build(){
            return std::make_shared<Logger>(logger_name_,flushs_,async_type);
        }

        private:
        std::string logger_name_;   //日志器名字
        std::vector<LogFlush::ptr> flushs_;      //写日志的方式
        AsyncType async_type = AsyncType::ASYNC_SAFE;    //  控制缓冲区是否增长
    };
}