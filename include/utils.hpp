#pragma once
namespace freelibcxx
{
template <typename T> static constexpr inline bool is_pow_of_2(T n) { return (n & (n - 1)) == 0; }

} // namespace freelibcxx