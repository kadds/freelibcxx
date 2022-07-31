#pragma once

#include "allocator.hpp"
#include <cstring>

class LibAllocator : public freelibcxx::Allocator
{
  public:
    void *allocate(size_t size, size_t align) noexcept
    {
        void *p = new char[size];
        memset(p, 1, size);
        return p;
    }
    void deallocate(void *p) noexcept { delete[](char *) p; }
};
extern LibAllocator LibAllocatorV;

struct Int
{
    Int(int i, int s = 0)
        : inner(new int(i))
        , v(i)
        , s(s)
    {
    }

    Int(const Int &rhs)
        : inner(new int(*rhs.inner))
        , v(rhs.v)
        , s(rhs.s)
    {
    }

    Int(Int &&rhs)
        : inner(rhs.inner)
        , v(rhs.v)
        , s(rhs.s)
    {
        rhs.inner = nullptr;
        rhs.v = 0;
        rhs.s = 0;
    }

    Int &operator=(const Int &rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }
        if (inner)
        {
            delete inner;
        }
        inner = new int(*rhs.inner);
        v = rhs.v;
        s = rhs.s;

        return *this;
    }

    Int &operator=(Int &&rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }
        if (inner)
        {
            delete inner;
        }
        inner = rhs.inner;
        v = rhs.v;
        s = rhs.s;
        rhs.inner = nullptr;
        rhs.v = 0;
        rhs.s = 0;
        return *this;
    }

    bool operator==(const Int &rhs) const { return *inner == *rhs.inner && v == rhs.v && s == rhs.s; }

    bool operator!=(const Int &rhs) const { return !operator==(rhs); }

    ~Int()
    {
        if (inner)
        {
            delete inner;
        }
    }
    int *inner;
    int v;
    int s;
};
