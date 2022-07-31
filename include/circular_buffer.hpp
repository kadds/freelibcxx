#pragma once
#include "allocator.hpp"
#include <cstddef>

namespace freelibcxx
{

template <typename T> class circular_buffer
{
  private:
    T *buffer_;
    size_t length_;
    size_t read_off_;
    size_t write_off_;
    Allocator *allocator_;

  public:
    circular_buffer(Allocator *allocator, size_t size)
        : buffer_(nullptr)
        , length_(size)
        , read_off_(0)
        , write_off_(0)
        , allocator_(allocator)
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

    void write(const T &t) { write(&t, 1); }

    void write(const T *t, size_t len)
    {
        size_t max_off = write_off_ + len;
        size_t rest_off = 0;
        size_t new_write_off = max_off;
        if (max_off >= length_)
        {
            rest_off = max_off - length_;
            max_off = length_;
            new_write_off = rest_off;
        }

        for (size_t off = write_off_; off < max_off; off++, t++)
        {
            buffer_[off] = *t;
        }

        for (size_t off = 0; off < rest_off; off++, t++)
        {
            buffer_[off] = *t;
        }

        write_off_ = new_write_off;
    }

    size_t capacity() const { return length_; }

    bool full() const { return (write_off_ + 1) % length_ == read_off_; }

    bool empty() const { return read_off_ == write_off_; }

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

    bool read(T *t)
    {
        if (empty()) [[unlikely]]
        {
            return false;
        }
        *t = buffer_[read_off_];
        if (read_off_ + 1 >= length_)
        {
            read_off_ = 0;
        }
        else
        {
            read_off_++;
        }
        return true;
    }

    size_t read(T *t, size_t size)
    {
        if (empty()) [[unlikely]]
        {
            return 0;
        }
        size_t rest = 0;
        if (read_off_ < write_off_)
        {
            rest = write_off_ - read_off_;
        }
        else
        {
            rest = length_ - read_off_ + write_off_;
        }
        if (size > rest)
        {
            size = rest;
        }
        size_t max_off = read_off_ + size;
        size_t rest_off = 0;
        size_t new_read = max_off;
        if (max_off >= length_)
        {
            rest_off = size - (max_off - read_off_);
            max_off = length_;
            new_read = rest_off;
        }

        for (size_t off = read_off_; off < max_off; off++, t++)
        {
            *t = buffer_[off];
        }

        for (size_t off = 0; off < rest_off; off++, t++)
        {
            *t = buffer_[off];
        }
        read_off_ = new_read;
        return size;
    }
};

} // namespace freelibcxx
