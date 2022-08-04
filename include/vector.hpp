#pragma once
#include "allocator.hpp"
#include "assert.hpp"
#include "iterator.hpp"
#include "utils.hpp"
#include <utility>

namespace freelibcxx
{

class slice
{
  public:
  private:
};

/// A container like std::vector
template <typename E> class base_vector
{
    template <typename N> struct value_fn
    {
        N operator()(N val) { return val; }
    };
    template <typename N> struct prev_fn
    {
        N operator()(N val) { return val - 1; }
    };
    template <typename N> struct next_fn
    {
        N operator()(N val) { return val + 1; }
    };

    using CE = const E *;
    using NE = E *;

  public:
    using const_iterator = base_bidirectional_iterator<CE, value_fn<CE>, prev_fn<CE>, next_fn<CE>>;
    using iterator = base_bidirectional_iterator<NE, value_fn<NE>, prev_fn<NE>, next_fn<NE>>;

    base_vector(Allocator *allocator)
        : buffer_(nullptr)
        , count_(0)
        , cap_(0)
        , allocator_(allocator)
    {
    }

    base_vector(Allocator *allocator, std::initializer_list<E> il)
        : base_vector(allocator)
    {
        recapacity(il.size());
        count_ = il.size();
        size_t i = 0;
        for (E a : il)
        {
            new (buffer_ + i) E(a);
            i++;
        }
    }

    base_vector(const base_vector &rhs) { copy(rhs); }

    base_vector(base_vector &&rhs) { move(std::move(rhs)); }

    ~base_vector() { free(); }

    base_vector &operator=(const base_vector &rhs)
    {
        if (&rhs == this)
            return *this;
        free();
        copy(rhs);
        return *this;
    }

    base_vector &operator=(base_vector &&rhs)
    {
        if (this == &rhs)
            return *this;
        free();
        move(std::move(rhs));
        return *this;
    }

    template <typename... Args> iterator push_back(Args &&...args)
    {
        check_capacity(count_ + 1);
        new (buffer_ + count_) E(std::forward<Args>(args)...);
        count_++;
        return iterator(buffer_ + count_ - 1);
    }

    template <typename... Args> iterator push_front(Args &&...args)
    {
        return insert_at(0, std::forward<Args>(args)...);
    }

    E pop_back()
    {
        CXXASSERT(count_ > 0);
        E e = std::move(buffer_[count_ - 1]);
        remove_at(count_ - 1);
        return e;
    }

    E pop_front()
    {
        CXXASSERT(count_ > 0);
        E e = std::move(buffer_[0]);
        remove_at(0);
        return e;
    }

    E &back()
    {
        CXXASSERT(count_ > 0);
        return buffer_[count_ - 1];
    }
    E &front()
    {
        CXXASSERT(count_ > 0);
        return buffer_[0];
    }

    const E &back() const
    {
        CXXASSERT(count_ > 0);
        return buffer_[count_ - 1];
    }

    const E &front() const
    {
        CXXASSERT(count_ > 0);
        return buffer_[0];
    }

    // insert before
    template <typename... Args> iterator insert_at(size_t index, Args &&...args)
    {
        CXXASSERT(index <= count_);
        check_capacity(count_ + 1);
        if (count_ > 0) [[likely]]
        {
            new (buffer_ + count_) E(std::move(buffer_[count_ - 1]));
            for (size_t i = count_ - 1; i > index; i--)
                buffer_[i] = std::move(buffer_[i - 1]);
        }

        new (buffer_ + index) E(std::forward<Args>(args)...);
        count_++;
        return iterator(buffer_ + index);
    }

    /// insert before iter
    template <typename... Args> iterator insert(iterator iter, Args &&...args)
    {
        size_t index = iter.get() - buffer_;
        return insert_at(index, std::forward<Args>(args)...);
    }

    /// remove at index
    iterator remove_at(size_t index)
    {
        if (index >= count_) [[unlikely]]
            return end();

        for (size_t i = index; i + 1 < count_; i++)
            buffer_[i] = std::move(buffer_[i + 1]);

        buffer_[count_ - 1].~E();

        check_capacity(count_ - 1);
        count_--;
        return iterator(&buffer_[index]);
    }

    /// remove current iter
    iterator remove(iterator iter)
    {
        size_t index = iter.get() - buffer_;
        return remove_at(index);
    }

    bool empty() const { return count_ == 0; }

    size_t size() const { return count_; }

    size_t capacity() const { return cap_; }

    const_iterator begin() const { return const_iterator(buffer_); }

    const_iterator end() const { return const_iterator(buffer_ + count_); }

    iterator begin() { return iterator(buffer_); }

    iterator end() { return iterator(buffer_ + count_); }

    E &at(size_t index)
    {
        CXXASSERT(index < count_);
        return buffer_[index];
    }

    const E &at(size_t index) const
    {
        CXXASSERT(index < count_);
        return buffer_[index];
    }

    E &operator[](size_t index) { return at(index); }

    void fitcapacity() { recapacity(count_); }

    template <typename... Args> void expand(size_t element_count_, Args &&...args)
    {
        if (element_count_ <= count_) [[unlikely]]
            return;

        recapacity(element_count_);
        for (size_t i = count_; i < element_count_; i++)
            new (&buffer_[i]) E(std::forward<Args>(args)...);

        count_ = element_count_;
    }

    void ensure(size_t size)
    {
        if (size > cap_)
        {
            recapacity(size);
        }
    }

    void shrink(size_t element_count)
    {
        if (element_count >= count_) [[unlikely]]
            return;
        recapacity(element_count);
    }

    void resize(size_t size, E &&val)
    {
        if (size < count_) [[unlikely]]
            truncate(size);
        else if (size == count_) [[unlikely]]
            return;
        else
            expand(size, std::move(val));
    }

    void truncate(size_t size)
    {
        for (size_t i = size; i < count_; i++)
            buffer_[i].~E();
        count_ = size;
    }

    void remove_at(size_t index, size_t end_index)
    {
        CXXASSERT(index >= 0 && index <= end_index && end_index <= count_);
        size_t rm_cnt = end_index - index;

        for (size_t i = index; i + rm_cnt < count_; i++)
            buffer_[i] = std::move(buffer_[i + rm_cnt]);

        for (size_t i = 1; i <= rm_cnt; i++)
            buffer_[count_ - i].~E();

        check_capacity(count_ - rm_cnt);
        count_ -= rm_cnt;
    }

    /// remove current iter
    iterator remove(iterator beg, iterator end)
    {
        size_t index = beg.get() - buffer_;
        size_t end_index = end.get() - buffer_;
        return remove_at(index, end_index);
    }

    void clear() { truncate(0); }

    E *data() { return buffer_; }

    const E *data() const { return buffer_; }

  private:
    void check_capacity(size_t new_count)
    {
        if (cap_ == 0 && new_count > 0) [[unlikely]]
            recapacity(2);
        else if (new_count > cap_) [[unlikely]]
        {
            size_t x = cap_ * 3 / 2;
            if (x < new_count)
                x = new_count;

            recapacity(x);
        }
    }

    void free()
    {
        if (buffer_ != nullptr)
        {
            truncate(0);
            allocator_->deallocate(buffer_);
            buffer_ = nullptr;
            cap_ = 0;
        }
    }

    void move(base_vector &&rhs)
    {
        buffer_ = rhs.buffer_;
        count_ = rhs.count_;
        cap_ = rhs.cap_;
        allocator_ = rhs.allocator_;
        rhs.buffer_ = nullptr;
        rhs.count_ = 0;
        rhs.cap_ = 0;
    }

    void copy(const base_vector &rhs)
    {
        count_ = rhs.count_;
        cap_ = rhs.count_;
        allocator_ = rhs.allocator_;
        buffer_ = reinterpret_cast<E *>(allocator_->allocate(count_ * sizeof(E), alignof(E)));
        for (size_t i = 0; i < count_; i++)
            buffer_[i] = rhs.buffer_[i];
    }

    void recapacity(size_t element_count)
    {
        if (element_count == cap_)
            return;

        E *new_buffer = reinterpret_cast<E *>(allocator_->allocate(element_count * sizeof(E), alignof(E)));
        if (buffer_ != nullptr)
        {
            size_t c = min(element_count, count_);

            for (size_t i = 0; i < c; i++)
                new (new_buffer + i) E(std::move(buffer_[i]));

            allocator_->deallocate(buffer_);
        }
        buffer_ = new_buffer;
        cap_ = element_count;
    }

  private:
    E *buffer_;
    size_t count_;
    size_t cap_;
    Allocator *allocator_;
};
template <typename T> using vector = base_vector<T>;

} // namespace freelibcxx
