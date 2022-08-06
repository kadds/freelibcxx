#include "freelibcxx/bit_set.hpp"
namespace freelibcxx
{
size_t const_bit_set_operation::scan_lead(size_t offset, size_t max_len)
{
    size_t start_idx = offset / BITS;
    size_t start_bits = offset % BITS;
    auto v = data_[start_idx] & ~((1UL << start_bits) - 1);
    if (v != 0)
    {
        return __builtin_ffsl(v) - 1 + start_idx * BITS;
    }

    size_t end = offset + max_len - 1;
    size_t end_idx = end / BITS;
    auto *d = data_;
    for (size_t i = start_idx + 1; i < end_idx; i++)
    {
        if (d[i] != 0)
        {
            return __builtin_ffsl(d[i]) - 1 + i * BITS;
        }
    }

    auto e = data_[end_idx];
    if (e != 0)
    {
        return __builtin_ffsl(e) - 1 + end_idx * BITS;
    }
    return (size_t)-1;
}
size_t const_bit_set_operation::scan_zero(size_t offset, size_t max_len)
{
    const BaseType fit = (BaseType)(-1);

    size_t start_idx = offset / BITS;
    size_t start_bits = offset % BITS;
    auto v = (~data_[start_idx]) & ~((1UL << start_bits) - 1);
    if (v != 0)
    {
        return __builtin_ffsl(v) - 1 + start_idx * BITS;
    }

    size_t end = offset + max_len - 1;
    size_t end_idx = end / BITS;
    auto *d = data_;
    for (size_t i = start_idx + 1; i < end_idx; i++)
    {
        if (d[i] != fit)
        {
            return __builtin_ffsl(~d[i]) - 1 + i * BITS;
        }
    }

    auto e = data_[end_idx];
    if (e != fit)
        return __builtin_ffsl(~e) - 1 + end_idx * BITS;

    return fit;
}
} // namespace freelibcxx