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
    enum class AsyncType
    {
        ASYNC_SAFE,
        ASYNC_UNSAFE
    };

    class AsynscWorker
    {
    public:
        using ptr = std::shared_ptr<AsynscWorker>;
        AsynscWorker(AsyncType type, std::function<void(AsynscBuffer&)> callback)
            : type_(type), callback_(callback), thread_(&AsynscWorker::consumer, this)
        {
        }

        void Push(const char *data, size_t len)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            // 当是固定大小buffer时，需要等待缓冲区可写部分大于len
            if (type_ == AsyncType::ASYNC_SAFE)
            {
                cond_productor_.wait(lock, [&]()
                                     { return len <= buff_productor_.WriteableSize(); });
            }
            buff_productor_.Push(data, len);
            cond_consumer_.notify_one();
        }

    private:
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
                if (type_ == AsyncType::ASYNC_SAFE)
                    cond_productor_.notify_one();
                lock.unlock();
                callback_(buff_consumer_);
                buff_consumer_.Reset();
                if (stop_ && buff_productor_.Empty())
                    return;
            }
        }

    private:
        std::atomic<bool> stop_; // 用于控制异步工作器的启动
        std::function<void(AsynscBuffer &)> callback_;
        AsyncType type_;
        std::condition_variable cond_productor_;
        std::condition_variable cond_consumer_;
        AsynscBuffer buff_productor_;
        AsynscBuffer buff_consumer_;
        std::mutex mutex_;
        std::thread thread_;
    };
}