#pragma once
#include <vector>
#include <cstddef>
#include "Util.hpp"

extern mylog::Util::JsonData *g_conf_data;

namespace mylog
{
    class AsynscBuffer
    {
    public:
        AsynscBuffer() : buffer_(g_conf_data->buffer_size_), write_pos_(0), read_pos_(0)
        {
        }

        //判断buffer内是否没有新增的可写入项
        bool Empty() const
        {
            return write_pos_ == read_pos_;
        }

        //交换生产者与消费者buffer
        void Swap(AsynscBuffer &other)
        {
            buffer_.swap(other.buffer_);
            std::swap(write_pos_, other.write_pos_);
            std::swap(read_pos_, other.read_pos_);
        }

        //消费者处理完后重置buffer状态
        void Reset()
        {
            write_pos_ = 0;
            read_pos_ = 0;
        }

        //消费者写日志的起始位置
        const char *Begin() const
        {
            return &buffer_[read_pos_];
        }

        //生产者可用大小
        size_t WriteableSize() const
        {
            return buffer_.size() - write_pos_;
        }

        //消费者需要处理的大小
        size_t ReadableSize() const
        {
            return write_pos_ - read_pos_;
        }

        //生产者写入
        void Push(const char *data, size_t len)
        {
            BeEnough(len);
            std::copy(data, data + len, &buffer_[write_pos_]);
            write_pos_ += len;
        }

        //确保可变buffer时容量足够
        void BeEnough(size_t len)
        {
            if (WriteableSize() >= len)
                return;
            if (buffer_.size() < g_conf_data->threshould_)
            {
                buffer_.reserve(2 * buffer_.size() + len);
            }
            else
            {
                buffer_.reserve(buffer_.size() + len + g_conf_data->line_growth_);
            }
        }

    private:
        std::vector<char> buffer_;
        size_t write_pos_;
        size_t read_pos_;
    };
}