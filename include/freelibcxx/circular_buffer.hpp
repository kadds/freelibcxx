#pragma once
#include "freelibcxx/allocator.hpp"
#include "freelibcxx/utils.hpp"
#include <cstddef>

namespace freelibcxx
{

template <typename T> class circular_buffer
{
  private:
    size_t read_off_;
    T *buffer_;
    size_t length_;
    Allocator *allocator_;
    size_t write_off_;

  public:
    circular_buffer(Allocator *allocator, size_t size)
        : read_off_(0)
        , buffer_(nullptr)
        , length_(size)
        , allocator_(allocator)
        , write_off_(0)
    {
        if (size > 0)
            buffer_ = reinterpret_cast<T *>(allocator->allocate(size * sizeof(T), alignof(T)));
        else
            buffer_ = nullptr;
    }

    ~circular_buffer()
    {
        if (buffer_ != nullptr)
            allocator_->deallocate(buffer_);
    }

    circular_buffer &operator=(const circular_buffer &cb) = delete;
    circular_buffer(const circular_buffer &cb) = delete;

    bool write(const T &t) { return write(&t, 1) == 1; }

    size_t write(const T *t, size_t len)
    {
        size_t w_size = capacity_writeable();
        len = min(w_size, len);
        size_t offset = write_off_;

        for (size_t i = 0; i < len; i++)
        {
            if (offset >= length_) [[unlikely]]
            {
                offset = 0;
            }
            buffer_[offset++] = t[i];
        }
        write_off_ = offset;
        return len;
    }

    size_t capacity() const { return length_; }

    size_t capacity_writeable() const
    {
        if (write_off_ < read_off_)
        {
            return read_off_ - write_off_ - 1;
        }
        else
        {
            return length_ - write_off_ + read_off_ - 1;
        }
    }

    size_t capacity_readable() const
    {
        if (write_off_ < read_off_)
        {
            return length_ - read_off_ + write_off_;
        }
        else
        {
            return write_off_ - read_off_;
        }
    }

    bool full() const { return capacity_writeable() == 0; }

    bool empty() const { return capacity_writeable() == capacity() - 1; }

    bool last(T *t)
    {
        if (empty()) [[unlikely]]
        {
            return false;
        }
        size_t off = write_off_;
        if (off == 0)
        {
            off = length_;
        }
        *t = buffer_[off - 1];
        return true;
    }

    bool read(T *t) { return read(t, 1) == 1; }

    size_t read(T *t, size_t size)
    {
        size_t len = capacity_readable();
        size = min(len, size);

        size_t offset = read_off_;

        for (size_t i = 0; i < size; i++)
        {
            if (offset >= length_) [[unlikely]]
            {
                offset = 0;
            }
            t[i] = buffer_[offset++];
        }
        read_off_ = offset;

        return size;
    }
};

} // namespace freelibcxx
