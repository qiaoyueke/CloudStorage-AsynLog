#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <string>
#include <future>

namespace mylog
{
    class ThreadPool
    {
    public:
        ThreadPool(int count)
        {
            for (int i = 0; i < count; i++)
            {
                workers_.emplace_back(
                    [this]()
                    {
                        while (true)
                        {
                            std::function<void()> task;

                            {
                                std::unique_lock<std::mutex> lock(mutex_);
                                this->cv_.wait(lock, [this]()
                                               { return this->stop_ || !this->tasks_.empty(); });
                                if (this->stop_ && this->tasks_.empty())
                                    return;

                                task = std::move(this->tasks_.front());
                                tasks_.pop();
                            }
                            std::cout<<"task运行"<<std::endl;
                            task();
                        }
                    });
            }
        }

        template <class Func, class... Args>
        auto add_task(Func &&f, Args &&...args) -> std::future<typename std::result_of<Func(Args...)>::type>
        {
            std::cout<<"add task"<<std::endl;
            using return_type = typename std::result_of<Func(Args...)>::type;
            auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<Func>(f), std::forward<Args>(args)...));

            {
                std::unique_lock<std::mutex> lock(mutex_);

                if (stop_) // 如果线程池已停止，抛出异常
                    throw std::runtime_error("enqueue on stopped ThreadPool");

                tasks_.emplace([task]()
                               { (*task)(); });

                cv_.notify_one();
            }
            return task->get_future();
        }

        ~ThreadPool()
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                stop_ = true;
            }
            cv_.notify_all();
            for (std::thread &worker : workers_)
            {
                worker.join();
            }
        }

    private:
        std::vector<std::thread> workers_; // 线程们
        std::mutex mutex_;
        std::queue<std::function<void()>> tasks_;
        bool stop_ = false;
        std::condition_variable cv_;
    };
}