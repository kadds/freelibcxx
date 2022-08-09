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

} // namespace freelibcxx