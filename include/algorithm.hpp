#pragma once
#include "iterator.hpp"
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
} // namespace freelibcxx