#pragma once
#include "freelibcxx/assert.hpp"
#include "freelibcxx/iterator.hpp"
#include "freelibcxx/utils.hpp"
#include <cstddef>
#include <type_traits>

namespace freelibcxx
{
template <typename T> class span
{
    template <typename U> friend class span;

    struct value_fn
    {
        T *operator()(T *val) { return val; }
    };
    struct random_fn
    {
        T *operator[](ptrdiff_t index) { return val_ + index; }

        ptrdiff_t offset_of(T *val) { return val_ - val; }

        T *val_;
        random_fn(T *val)
            : val_(val)
        {
        }
    };

  public:
    span()
        : ptr_(nullptr)
        , size_(0)
    {
    }

    template <typename U>
    requires std::is_same_v<std::remove_cv_t<T>, U> span(const span<U> &rhs)
        : ptr_(reinterpret_cast<T *>(rhs.ptr_))
        , size_(rhs.size_)
    {
    }

    template <typename U>
    requires std::is_same_v<std::remove_cv_t<T>, U> span &operator=(const span<U> &rhs)
    {
        ptr_ = reinterpret_cast<T *>(rhs.ptr_);
        size_ = rhs.size_;
    }

    span(T *ptr, size_t size)
        : ptr_(ptr)
        , size_(size)
    {
    }

    span subspan(size_t pos, size_t size)
    {
        CXXASSERT(pos + size <= size_);
        CXXASSERT(pos < size_);

        return span(ptr_ + pos, size);
    }

    span subspan(size_t pos)
    {
        CXXASSERT(pos <= size_);

        return span(ptr_ + pos, size_ - pos);
    }

    size_t size() const { return size_; }
    T *get() const { return ptr_; }

    T &operator[](size_t index) { return ptr_[index]; }

    T operator[](size_t index) const { return ptr_[index]; }

  private:
    T *ptr_;
    size_t size_;
};
} // namespace freelibcxx