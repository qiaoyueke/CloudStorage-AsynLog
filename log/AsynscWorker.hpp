#pragma once
#include <memory>
#include <functional>
#include <condition_variable>
#include <thread>
#include <string>
#include <atomic>
#include "AsynscBuffer.hpp"

namespace mylog
{
    enum class BufferType
    {
        //缓冲区大小是否可变
        UNCHANGEABLE,     //buffer大小不可变
        CHANGEABLE        //buffer可变
    };

    class AsynscWorker
    {
    public:
        using ptr = std::shared_ptr<AsynscWorker>;
        AsynscWorker(BufferType type, std::function<void(AsynscBuffer &)> callback)
            : type_(type), callback_(callback), thread_(&AsynscWorker::consumer, this), stop_(false)
        {
        }

        ~AsynscWorker()
        {
            Stop();
        }

        //停止异步工作者写日志
        void Stop()
        {
            stop_ = true;
            cond_consumer_.notify_all(); // 所有线程把缓冲区内数据处理完就结束了
            thread_.join();
        }

        //生产者实际将内容写入buffer
        void Push(const char *data, size_t len)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            // 当是固定大小buffer时，需要等待缓冲区可写部分大于len
            if (type_ == BufferType::UNCHANGEABLE)
            {
                cond_productor_.wait(lock, [&]()
                                     { return len <= buff_productor_.WriteableSize(); });
            }
            buff_productor_.Push(data, len);
            cond_consumer_.notify_one();
        }

    private:
        //消费者线程执行的函数
        void consumer()
        {
            while (true)
            {
                std::unique_lock<std::mutex> lock(mutex_);
                // 在没有要求停止时，生产者缓冲区为空，则阻塞
                if (!stop_ && buff_productor_.Empty())
                {
                    cond_consumer_.wait(lock, [&]()
                                        { return stop_ || !buff_productor_.Empty(); });
                }
                buff_consumer_.Swap(buff_productor_);
                //交换完就解锁，当buffer大小不可变时唤醒生产者
                lock.unlock();
                if (type_ == BufferType::UNCHANGEABLE)
                    cond_productor_.notify_one();
                //实际将buffer内容读出后，重置buffer状态
                callback_(buff_consumer_);
                buff_consumer_.Reset();

                //要求停止时，等两个缓冲区都空后，停止消费者线程
                if (stop_ && buff_productor_.Empty())
                    return;
            }
        }

    private:
        std::atomic<bool> stop_; // 用于控制异步工作器的启动
        std::function<void(AsynscBuffer &)> callback_;
        BufferType type_;
        std::condition_variable cond_productor_;
        std::condition_variable cond_consumer_;
        AsynscBuffer buff_productor_;
        AsynscBuffer buff_consumer_;
        std::mutex mutex_;
        std::thread thread_;
    };
}