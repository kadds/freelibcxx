#pragma once
#include "allocator.hpp"
#include "extern.hpp"
#include <cstddef>

namespace freelibcxx
{

template <typename B = size_t> class bit_set_operation
{
    using BaseType = B;
    static constexpr size_t bits = sizeof(BaseType) * 8;

  public:
    bit_set_operation(B *ptr)
        : data_(ptr)
    {
    }

    bool get(size_t index)
    {
        auto idx = index / bits;
        auto rest = index % bits;
        auto v = data_[idx];
        v >>= rest;
        return v & 1;
    }

    void set(size_t index)
    {
        auto idx = index / bits;
        auto rest = index % bits;
        data_[idx] |= ((1ul << rest));
    }
    /// set [0, element_count) to 1
    void set_all(size_t element_count)
    {
        size_t bytes = (element_count + 7) / 8;
        memset(data_, -1, bytes);
    }
    /// set [start, start+element_count) to 1
    void set_all(size_t start, size_t element_count)
    {
        size_t start_idx = start / bits;
        size_t start_rest = start % bits;
        data_[start_idx] |= ~((1ul << start_rest) - 1);
        size_t end = start + element_count - 1;
        size_t end_idx = end / bits;
        size_t end_rest = end % bits;
        data_[start_idx] |= ((1ul << end_rest) - 1);

        size_t bytes = (end_idx - start_idx) * 8;
        memset(data_, -1, bytes);
    }

    void clean(size_t index)
    {
        auto idx = index / bits;
        auto rest = index % bits;
        data_[idx] &= ~((1ul << rest));
    }
    /// set [0, element_count) to 0
    void clean_all(size_t element_count)
    {
        size_t bytes = (element_count + 7) / 8;
        memset(data_, 0, bytes);
    }
    /// set [start, start+element_count) to 0
    void clean_all(size_t start, size_t element_count)
    {
        size_t start_idx = start / bits;
        size_t start_rest = start % bits;
        data_[start_idx] &= ((1ul << (start_rest)) - 1);
        size_t end = start + element_count - 1;
        size_t end_idx = end / bits;
        size_t end_rest = end % bits;
        data_[start_idx] &= ~((1ul << (end_rest)) - 1);

        size_t bytes = (end_idx - start_idx) * 8;
        memset(data_, 0, bytes);
    }
    /// scan [offset, offset+max_len)
    size_t scan_zero(size_t offset, size_t max_len)
    {
        const BaseType fit = (BaseType)(-1);

        size_t start_idx = offset / bits;
        size_t start_rest = offset % bits;
        auto v = (~data_[start_idx]) & ~((1ul << start_rest) - 1);
        if (v != 0)
        {
            return __builtin_ffsl(v) - 1 + start_idx * bits;
        }

        size_t end = offset + max_len - 1;
        size_t end_idx = end / bits;
        auto *d = data_;
        for (size_t i = start_idx + 1; i < end_idx; i++)
        {
            if (d[i] != fit)
            {
                return __builtin_ffsl(~d[i]) - 1 + i * bits;
            }
        }

        auto e = data_[end_idx];
        if (e != fit)
            return __builtin_ffsl(~e) - 1 + end_idx * bits;

        return fit;
    }

    size_t scan_lead(size_t offset, size_t max_len)
    {
        size_t start_idx = offset / bits;
        size_t start_rest = offset % bits;
        auto v = data_[start_idx] & ~((1ul << start_rest) - 1);
        if (v != 0)
        {
            return __builtin_ffsl(v) - 1 + start_idx * bits;
        }

        size_t end = offset + max_len - 1;
        size_t end_idx = end / bits;
        auto *d = data_;
        for (size_t i = start_idx + 1; i < end_idx; i++)
        {
            if (d[i] != 0)
            {
                return __builtin_ffsl(d[i]) - 1 + i * bits;
            }
        }

        auto e = data_[end_idx];
        if (e != 0)
        {
            return __builtin_ffsl(e) - 1 + end_idx * bits;
        }
        return (size_t)-1;
    }

  private:
    BaseType *data_;
};

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

        size_t bytes = (element_count + 7) / 8;
        data_ = reinterpret_cast<size_t *>(allocator->allocate(bytes, 8));
    }

    bit_set(const bit_set &v) = delete;
    bit_set &operator=(const bit_set &v) = delete;

    ~bit_set() { allocator_->deallocate(data_); }

    void clean_all()
    {
        bit_set_operation<size_t> op(data_);
        op.clean_all(element_count_);
    }

    void set_all()
    {
        bit_set_operation<size_t> op(data_);
        op.set_all(element_count_);
    }

    bool get(size_t index)
    {
        bit_set_operation<size_t> op(data_);
        return op.get(index);
    }

    void set(size_t index)
    {
        bit_set_operation<size_t> op(data_);
        op.set(index);
    }

    void set_all(size_t index, size_t count)
    {
        bit_set_operation<size_t> op(data_);
        op.set_all(index, count);
    }

    void clean(size_t index)
    {
        bit_set_operation<size_t> op(data_);
        op.clean(index);
    }

    void clean_all(size_t index, size_t count)
    {
        bit_set_operation<size_t> op(data_);
        op.clean_all(index, count);
    }

    size_t scan_zero(size_t offset, size_t max_len)
    {
        bit_set_operation<size_t> op(data_);
        return op.scan_zero(offset, max_len);
    }

    size_t scan_zero()
    {
        bit_set_operation<size_t> op(data_);
        return op.scan_zero(0, element_count_);
    };

    size_t scan_lead(size_t offset, size_t max_len)
    {
        bit_set_operation<size_t> op(data_);
        return op.scan_lead(offset, max_len);
    }

    size_t scan_lead()
    {
        bit_set_operation<size_t> op(data_);
        return op.scan_lead(0, element_count_);
    }

    size_t count() const { return element_count_; }
};

template <int ElementCount> class bit_set_inplace
{
    size_t data_[(ElementCount + 63) / 64];

  public:
    bit_set_inplace() = default;
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
    void clean_all()
    {
        bit_set_operation<size_t> op(data_);
        op.clean_all(ElementCount);
    }

    void set_all()
    {
        bit_set_operation<size_t> op(data_);
        op.set_all(ElementCount);
    }

    bool get(size_t index)
    {
        bit_set_operation<size_t> op(data_);
        return op.get(index);
    }

    void set(size_t index)
    {
        bit_set_operation<size_t> op(data_);
        op.set(index);
    }

    void set_all(size_t index, size_t count)
    {
        bit_set_operation<size_t> op(data_);
        op.set_all(index, count);
    }

    void clean(size_t index)
    {
        bit_set_operation<size_t> op(data_);
        op.clean(index);
    }

    void clean_all(size_t index, size_t count)
    {
        bit_set_operation<size_t> op(data_);
        op.clean_all(index, count);
    }

    size_t scan_zero(size_t offset, size_t max_len)
    {
        bit_set_operation<size_t> op(data_);
        return op.scan_zero(offset, max_len);
    }

    size_t scan_zero()
    {
        bit_set_operation<size_t> op(data_);
        return op.scan_zero(0, ElementCount);
    };

    size_t scan_lead(size_t offset, size_t max_len)
    {
        bit_set_operation<size_t> op(data_);
        return op.scan_lead(offset, max_len);
    }

    size_t scan_lead()
    {
        bit_set_operation<size_t> op(data_);
        return op.scan_lead(0, ElementCount);
    }

    size_t count() const { return ElementCount; }
};
} // namespace freelibcxx
