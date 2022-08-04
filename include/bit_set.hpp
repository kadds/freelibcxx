#pragma once
#include "allocator.hpp"
#include "extern.hpp"
#include "utils.hpp"
#include <cstddef>

namespace freelibcxx
{

class const_bit_set_operation
{
    using BaseType = size_t;
    static constexpr size_t BITS = sizeof(BaseType) * 8;
    static_assert(sizeof(size_t) == sizeof(long));

  public:
    const_bit_set_operation(const BaseType *ptr)
        : data_(ptr)
    {
    }

    bool get_bit(size_t index) const
    {
        auto idx = index / BITS;
        auto bits = index % BITS;
        return (data_[idx] >> bits) & 1;
    }

    /// scan [offset, offset+max_len)
    size_t scan_zero(size_t offset, size_t max_len);

    size_t scan_lead(size_t offset, size_t max_len);

  private:
    const BaseType *data_;
};

class bit_set_operation
{
    using BaseType = size_t;
    static constexpr size_t BITS = sizeof(BaseType) * 8;

  public:
    bit_set_operation(BaseType *ptr)
        : data_(ptr)
    {
    }

    void set_bit(size_t index)
    {
        auto idx = index / BITS;
        auto bits = index % BITS;
        data_[idx] |= ((1UL << bits));
    }

    void reset_bit(size_t index)
    {
        auto idx = index / BITS;
        auto bits = index % BITS;
        data_[idx] &= ~((1UL << bits));
    }

    void xor_bit(size_t index, bool val)
    {
        auto idx = index / BITS;
        auto bits = index % BITS;
        data_[idx] ^= (((size_t)(val) << bits));
    }

    /// set [0, element_count) to 1
    void set_all(size_t element_count) { set_all(0, element_count); }

    /// set [start, start+element_count) to 1
    void set_all(size_t start, size_t element_count)
    {
        size_t start_idx = start / BITS;
        size_t start_bits = start % BITS;
        data_[start_idx] |= ~((1UL << start_bits) - 1);

        size_t end = start + element_count;
        size_t end_idx = end / BITS;
        size_t end_bits = end % BITS;
        if (end_bits > 0)
        {
            data_[end_idx] |= ((1UL << end_bits) - 1);
            end_idx--;
        }
        if (end_idx < ++start_idx)
        {
            return;
        }

        size_t bytes = (end_idx - start_idx) * sizeof(BaseType);
        memset(data_ + start_idx, -1, bytes);
    }

    /// set [0, element_count) to 0
    void reset_all(size_t element_count) { reset_all(0, element_count); }

    /// set [start, start+element_count) to 0
    void reset_all(size_t start, size_t element_count)
    {
        size_t start_idx = start / BITS;
        size_t start_bits = start % BITS;
        data_[start_idx] &= ((1UL << start_bits) - 1);

        size_t end = start + element_count;
        size_t end_idx = end / BITS;
        size_t end_bits = end % BITS;
        if (end_bits > 0)
        {
            data_[end_idx] &= ~((1UL << end_bits) - 1);
            end_idx--;
        }

        if (end_idx < ++start_idx)
        {
            return;
        }

        size_t bytes = (end_idx - start_idx) * sizeof(BaseType);
        memset(data_ + start_idx, 0, bytes);
    }

  private:
    BaseType *data_;
};

#define FREELIBCXX_BITSET_FN                                                                                           \
    void reset_all()                                                                                                   \
    {                                                                                                                  \
        bit_set_operation op(data_);                                                                                   \
        op.reset_all(element_count_);                                                                                  \
    }                                                                                                                  \
    void set_all()                                                                                                     \
    {                                                                                                                  \
        bit_set_operation op(data_);                                                                                   \
        op.set_all(element_count_);                                                                                    \
    }                                                                                                                  \
    bool get_bit(size_t index) const                                                                                   \
    {                                                                                                                  \
        const_bit_set_operation op(data_);                                                                             \
        return op.get_bit(index);                                                                                      \
    }                                                                                                                  \
    bool operator[](size_t index) const { return get_bit(index); }                                                     \
    void set_bit(size_t index)                                                                                         \
    {                                                                                                                  \
        bit_set_operation op(data_);                                                                                   \
        op.set_bit(index);                                                                                             \
    }                                                                                                                  \
    void set_all(size_t index, size_t count)                                                                           \
    {                                                                                                                  \
        bit_set_operation op(data_);                                                                                   \
        op.set_all(index, count);                                                                                      \
    }                                                                                                                  \
    void xor_bit(size_t index, bool val)                                                                               \
    {                                                                                                                  \
        bit_set_operation op(data_);                                                                                   \
        op.xor_bit(index, val);                                                                                        \
    }                                                                                                                  \
    void reset_bit(size_t index)                                                                                       \
    {                                                                                                                  \
        bit_set_operation op(data_);                                                                                   \
        op.reset_bit(index);                                                                                           \
    }                                                                                                                  \
    void reset_all(size_t index, size_t count)                                                                         \
    {                                                                                                                  \
        bit_set_operation op(data_);                                                                                   \
        op.reset_all(index, count);                                                                                    \
    }                                                                                                                  \
    size_t scan_zero(size_t offset, size_t max_len) const                                                              \
    {                                                                                                                  \
        const_bit_set_operation op(data_);                                                                             \
        return op.scan_zero(offset, max_len);                                                                          \
    }                                                                                                                  \
    size_t scan_zero() const                                                                                           \
    {                                                                                                                  \
        const_bit_set_operation op(data_);                                                                             \
        return op.scan_zero(0, element_count_);                                                                        \
    }                                                                                                                  \
    size_t scan_lead(size_t offset, size_t max_len) const                                                              \
    {                                                                                                                  \
        const_bit_set_operation op(data_);                                                                             \
        return op.scan_lead(offset, max_len);                                                                          \
    }                                                                                                                  \
    size_t scan_lead() const                                                                                           \
    {                                                                                                                  \
        const_bit_set_operation op(data_);                                                                             \
        return op.scan_lead(0, element_count_);                                                                        \
    }                                                                                                                  \
    size_t count() const { return element_count_; }

class bit_set
{
  private:
    // data pointer
    size_t *data_;
    size_t element_count_;
    Allocator *allocator_;

  public:
    bit_set(Allocator *allocator, size_t element_count)
        : element_count_(element_count)
        , allocator_(allocator)
    {
        CXXASSERT(is_pow_of_2(element_count));
        size_t bytes = (element_count + 7) / 8;
        data_ = reinterpret_cast<size_t *>(allocator->allocate(bytes, 8));
    }

    bit_set(const bit_set &v) = delete;
    bit_set &operator=(const bit_set &v) = delete;

    ~bit_set() { allocator_->deallocate(data_); }

    FREELIBCXX_BITSET_FN;
};

template <size_t ElementCount> class bit_set_inplace
{
    size_t data_[(ElementCount + 63) / 64];
    static constexpr size_t element_count_ = ElementCount;
    static_assert(is_pow_of_2(ElementCount));

  public:
    bit_set_inplace() { memset(data_, 0, sizeof(data_)); };
    bit_set_inplace(size_t map) { set_to(map); }

    ~bit_set_inplace() = default;

    size_t *get_ptr() { return data_; }

    void set_to(size_t map)
    {
        if (ElementCount > sizeof(size_t) * 8)
        {
            /// FIXME: set mask
        }
        data_[0] = map;
    }

    FREELIBCXX_BITSET_FN;
};
} // namespace freelibcxx
