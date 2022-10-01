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

template <typename T, typename E>
requires forward_iterator<T> T find(T beg, T end, const E &val)
{
    while (beg != end)
    {
        if (*beg == val)
        {
            return beg;
        }
        beg++;
    }
    return beg;
}

template <typename T, typename P>
requires forward_iterator<T> T find_if(T beg, T end, P p)
{
    while (beg != end)
    {
        if (p(*beg))
        {
            return beg;
        }
        beg++;
    }
    return beg;
}

template <typename T, typename E>
requires random_access_iterator<T> T lower_bound(T beg, T end, const E &val)
{
    while (beg != end)
    {
        auto m = beg + (end - beg) / 2;
        if (*m < val)
        {
            beg = m + 1;
        }
        else
        {
            end = m;
        }
    }
    return beg;
}

template <typename T, typename E>
requires random_access_iterator<T> T upper_bound(T beg, T end, const E &val)
{
    while (beg != end)
    {
        auto m = beg + (end - beg) / 2;
        if (*m <= val)
        {
            beg = m + 1;
        }
        else
        {
            end = m;
        }
    }
    return beg;
}

template <typename T>
requires random_access_iterator<T>
void sort(T beg, T end)
{
    if (beg >= end)
    {
        return;
    }
    auto low = beg;
    auto high = end - 1;

    // quick sort
    auto pilot = *beg;
    while (low < high)
    {
        while (*high >= pilot && high > low)
            --high;
        if (high > low)
        {
            *low = *high;
        }
        while (*low <= pilot && high > low)
            ++low;
        if (high > low)
        {
            *high = *low;
        }
    }
    *low = pilot;
    sort(beg, low - 1);
    sort(low + 1, end);
}

} // namespace freelibcxx