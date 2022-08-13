#pragma once
#include "freelibcxx/iterator.hpp"
#include <stddef.h>
#include <stdint.h>

namespace freelibcxx
{
template <typename T>
requires bidirectional_iterator<T>
void reverse(T begin, T end)
{
    while (begin != end)
    {
        std::swap(*begin, *--end);

        if (begin == end)
        {
            break;
        }
        begin++;
    }
}
static constexpr inline size_t next_pow_of_2(size_t t) { return t <= 1 ? 1 : 1UL << (64 - __builtin_clzl(t - 1)); }
static constexpr inline size_t cur_pow_of_2(size_t t) { return t <= 1 ? 1 : 1UL << (64 - __builtin_clzl(t - 1) - 1); }

static constexpr inline size_t select_capacity(size_t capacity)
{
    if (capacity >= (1UL << 25))
    {
        return cur_pow_of_2(capacity) + (1UL << 25);
    }
    return next_pow_of_2(capacity);
}

static constexpr inline bool is_space(char ch) { return ch == ' ' || ch == '\t'; }

} // namespace freelibcxx