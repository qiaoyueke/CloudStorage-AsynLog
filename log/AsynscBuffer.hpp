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

        bool Empty() const
        {
            return write_pos_ == 0;
        }

        void Swap(AsynscBuffer &other)
        {
            buffer_.swap(other.buffer_);
            std::swap(write_pos_, other.write_pos_);
            std::swap(read_pos_, other.read_pos_);
        }

        void Reset()
        {
            write_pos_ = 0;
            read_pos_ = 0;
        }

        const char *Begin() const
        {
            return &buffer_[read_pos_];
        }

        size_t WriteableSize() const
        {
            return buffer_.size() - write_pos_;
        }

        size_t ReadableSize() const
        {
            return write_pos_ - read_pos_;
        }

        void Push(const char *data, size_t len)
        {
            BeEnough(len);
            std::cout << "写入buffer" << std::endl;
            std::copy(data, data + len, &buffer_[write_pos_]);
            write_pos_ += len;
        }

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