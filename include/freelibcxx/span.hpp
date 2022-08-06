#pragma once
#include "freelibcxx/assert.hpp"
#include "freelibcxx/utils.hpp"
#include <cstddef>
#include <type_traits>
namespace freelibcxx
{
template <typename T> class span
{
    template <typename U> friend class span;

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
        CXXASSERT(pos + size < size_);
        CXXASSERT(pos < size_);

        return span(ptr_ + pos, size);
    }

    span subspan(size_t pos)
    {
        CXXASSERT(pos < size_);

        return span(ptr_ + pos, size_ - pos);
    }

    size_t size() const { return size_; }
    T *get() const { return ptr_; }

    char &operator[](size_t index) { return ptr_[index]; }

  private:
    T *ptr_;
    size_t size_;
};
} // namespace freelibcxx