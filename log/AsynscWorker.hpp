#pragma once
#include<memory>
#include<functional>
#include<condition_variable>
#include<thread>
#include<string>
#include"AsynscBuffer.hpp"

namespace log{
    enum class AsyncType{ASYNC_SAFE, ASYNC_UNSAFE};

    class AsynscWorker
    {
        public:
        using ptr = std::shared_ptr<AsynscWorker>;
        AsynscWorker(AsyncType type, std::function<void()> callback)
        :type_(type), callback_(callback), thread_(consumer,this)
        {}


        private:
        void consumer(){
            while(true){
                callback_();
            }
        }


        private:
        std::function<void()> callback_;
        AsyncType type_;
        std::condition_variable cond_productor_;
        std::condition_variable cond_consumer_;
        AsynscBuffer buff_productor_;
        AsynscBuffer buff_consumer_;
        std::mutex mutex_;
        std::thread thread_;


    };
}