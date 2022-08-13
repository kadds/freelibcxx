#pragma once
#include "freelibcxx/algorithm.hpp"
#include "freelibcxx/allocator.hpp"
#include "freelibcxx/assert.hpp"
#include "freelibcxx/iterator.hpp"
#include "utils.hpp"
#include <cstddef>
#include <type_traits>
#include <utility>

namespace freelibcxx
{

/// A container like std::vector
template <typename E> class base_vector
{
    template <typename N> struct value_fn
    {
        N operator()(N val) { return val; }
    };

    template <typename N> struct random_fn
    {
        N operator[](ptrdiff_t index) { return val_ + index; }

        ptrdiff_t offset_of(N val) { return val_ - val; }

        N val_;
        random_fn(N val)
            : val_(val)
        {
        }
    };

    using CE = const E *;
    using NE = E *;

  public:
    using const_iterator = base_random_access_iterator<CE, value_fn<CE>, random_fn<CE>>;
    using iterator = base_random_access_iterator<NE, value_fn<NE>, random_fn<NE>>;

    base_vector(Allocator *allocator)
        : buffer_(nullptr)
        , count_(0)
        , cap_(0)
        , allocator_(allocator)
    {
    }

    base_vector(Allocator *allocator, std::initializer_list<E> ilist)
        : base_vector(allocator)
    {
        recapacity(ilist.size());
        size_t i = 0;
        for (const E &a : ilist)
        {
            new (buffer_ + i++) E(a);
        }
        count_ = i;
    }

    base_vector(const base_vector &rhs) { copy(rhs); }

    base_vector(base_vector &&rhs) noexcept { move(std::move(rhs)); }

    ~base_vector() { free(); }

    base_vector &operator=(const base_vector &rhs)
    {
        if (this == &rhs) [[unlikely]]
            return *this;
        free();
        copy(rhs);
        return *this;
    }

    base_vector &operator=(base_vector &&rhs) noexcept
    {
        if (this == &rhs) [[unlikely]]
            return *this;
        free();
        move(std::move(rhs));
        return *this;
    }

    template <typename... Args> iterator push_back(Args &&...args)
    {
        ensure(count_ + 1);
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
        buffer_[count_ - 1].~E();
        count_--;
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
        ensure(count_ + 1);
        if (count_ > 0) [[likely]]
        {
            if constexpr (std::is_nothrow_move_constructible_v<E>)
            {
                new (buffer_ + count_) E(std::move(buffer_[count_ - 1]));
            }
            else
            {
                new (buffer_ + count_) E(buffer_[count_ - 1]);
            }
            for (size_t i = count_ - 1; i > index; i--)
            {
                if constexpr (std::is_nothrow_move_assignable_v<E>)
                {
                    buffer_[i] = std::move(buffer_[i - 1]);
                }
                else
                {
                    buffer_[i] = buffer_[i - 1];
                }
            }
        }
        if (index < count_)
        {
            buffer_[index].~E();
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
        remove_at(index, index + 1);
        return begin() + index;
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

    void expand(size_t element_count, E &&val)
    {
        if (element_count <= count_) [[unlikely]]
            return;

        ensure(element_count);
        for (size_t i = count_; i < element_count; i++)
        {
            new (&buffer_[i]) E(val);
        }

        count_ = element_count;
    }

    void shrink(size_t element_count)
    {
        if (element_count > count_) [[unlikely]]
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
        CXXASSERT(size <= count_);
        for (size_t i = size; i < count_; i++)
        {
            buffer_[i].~E();
        }
        count_ = size;
    }

    void remove_at(size_t index, size_t end_index)
    {
        CXXASSERT(index >= 0 && index <= end_index && end_index <= count_);
        size_t remove_count = end_index - index;

        for (size_t i = index + remove_count; i < count_; i++)
        {
            if constexpr (std::is_nothrow_move_assignable_v<E>)
            {
                buffer_[i - remove_count] = std::move(buffer_[i]);
            }
            else
            {
                buffer_[i - remove_count] = buffer_[i];
            }
        }

        for (size_t i = 1; i <= remove_count; i++)
        {
            buffer_[count_ - remove_count].~E();
        }

        count_ -= remove_count;
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

    void ensure(size_t new_cap)
    {
        if (new_cap > cap_)
        {
            recapacity(select_capacity(new_cap));
        }
    }

  private:
    void free() noexcept
    {
        if (buffer_ != nullptr)
        {
            truncate(0);
            allocator_->deallocate(buffer_);
            buffer_ = nullptr;
            cap_ = 0;
        }
    }

    void move(base_vector &&rhs) noexcept
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
        {
            new (buffer_ + i) E(rhs.buffer_[i]);
        }
    }

    void recapacity(size_t cap)
    {
        if (cap == cap_)
            return;

        E *buffer = reinterpret_cast<E *>(allocator_->allocate(cap * sizeof(E), alignof(E)));
        if (buffer_ != nullptr)
        {
            for (size_t i = 0; i < count_; i++)
            {
                if constexpr (std::is_nothrow_move_constructible_v<E>)
                {
                    new (buffer + i) E(std::move(buffer_[i]));
                }
                else
                {
                    new (buffer + i) E(buffer_[i]);
                }
                buffer_[i].~E();
            }

            allocator_->deallocate(buffer_);
        }
        buffer_ = buffer;
        cap_ = cap;
    }

  private:
    E *buffer_;
    size_t count_;
    size_t cap_;
    Allocator *allocator_;
};
template <typename T> using vector = base_vector<T>;

} // namespace freelibcxx
