#pragma once
#include "freelibcxx/assert.hpp"
#include <cstddef>
#include <cstdint>
#include <new>

// extern "C" [[gnu::weak]] void *aligned_alloc(size_t alignment, size_t size) noexcept;
// extern "C" [[gnu::weak]] void free(void *ptr) noexcept;

#include <utility>
namespace freelibcxx
{
class Allocator
{
  public:
    Allocator(){};
    virtual ~Allocator(){};
    /// Allocate a memory address.
    /// Default is virtual address.
    ///
    /// \param size The memory size which you want to allocate
    /// \param align The pointer alignment value
    /// \return Return the address has been allocate
    virtual void *allocate(size_t size, size_t align) noexcept = 0;
    /// Deallocate a memory address.
    /// Default is virtual address.
    ///
    /// \param ptr The address which you want to deallocate
    /// \return None
    virtual void deallocate(void *ptr) noexcept = 0;

    template <typename T, typename... Args> T *New(Args &&...args) noexcept
    {
        auto ptr = allocate(sizeof(T), alignof(T));
        if (!ptr) [[unlikely]]
        {
            return nullptr;
        }
        auto k = new (ptr) T(std::forward<Args>(args)...);
        return k;
    }
    template <typename T, typename... Args> T *NewArray(size_t n, Args &&...args) noexcept
    {
        auto ptr = allocate(sizeof(T) * n, alignof(T));
        if (!ptr) [[unlikely]]
        {
            return nullptr;
        }
        auto t = reinterpret_cast<T *>(ptr);
        for (size_t i = 0; i < n; i++)
        {
            new (t + i) T(std::forward<Args>(args)...);
        }
        return t;
    }

    template <typename T> void Delete(T *t) noexcept
    {
        t->~T();
        deallocate(t);
    }

    template <typename T> void DeleteArray(size_t n, T *t) noexcept
    {
        for (size_t i = 0; i < n; i++)
        {
            t[i].~T();
        }
        deallocate(t);
    }
};

class NullAllocator : public Allocator
{
  public:
    void *allocate(size_t size, size_t align) noexcept override { return nullptr; }
    void deallocate(void *ptr) noexcept override {}
};

class CustomAllocator : public Allocator
{
  public:
    CustomAllocator(void *p, size_t s)
        : p_(p)
    {
    }
    void *allocate(size_t size, size_t align) noexcept override
    {
        CXXASSERT(size <= s_);
        return p_;
    }
    void deallocate(void *ptr) noexcept override {}

  private:
    void *p_;
    size_t s_;
};

} // namespace freelibcxx