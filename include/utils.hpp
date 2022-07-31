#pragma once
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

} // namespace freelibcxx