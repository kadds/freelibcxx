#pragma once
#include "assert.hpp"
#include "utils.hpp"
#include <cstddef>
namespace freelibcxx
{
template <typename T> class span
{
  public:
    span()
        : ptr_(nullptr)
        , size_(0)
    {
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
    size_t size() const { return size_; }
    T *get() const { return ptr_; }

  private:
    T *ptr_;
    size_t size_;
};
} // namespace freelibcxx