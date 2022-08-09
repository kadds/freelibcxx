#pragma once
#include <limits>
namespace freelibcxx
{
template <typename T> static constexpr inline bool is_pow_of_2(T n) { return (n & (n - 1)) == 0; }

template <typename T> static constexpr inline T min(T l, T r) { return l < r ? l : r; }
template <typename T> static constexpr inline T max(T l, T r) { return l > r ? l : r; }
template <typename T> static constexpr inline T clamp(T val, T min, T max)
{
    if (val < min)
    {
        return min;
    }
    if (val > max)
    {
        return max;
    }
    return val;
}

template <typename T, typename V> static constexpr inline bool in_range_of_type(V val)
{
    return val >= std::numeric_limits<T>::min() && val <= std::numeric_limits<T>::max();
}

static constexpr inline char tolower(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c + ('z' - 'Z');
    }
    return c;
}

static constexpr inline char toupper(char c)
{
    if (c >= 'a' && c <= 'z')
    {
        return c - ('z' - 'Z');
    }
    return c;
}

} // namespace freelibcxx