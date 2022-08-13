#pragma once
#include "freelibcxx/allocator.hpp"
#include "freelibcxx/assert.hpp"
#include "freelibcxx/iterator.hpp"
#include <utility>

namespace freelibcxx
{
template <typename E> class linked_list
{
  public:
    struct list_node;

  private:
    template <typename N, typename K> struct value_fn
    {
        K operator()(N val) { return &val->element; }
    };
    template <typename N> struct prev_fn
    {
        N operator()(N val) { return val->prev; }
    };
    template <typename N> struct next_fn
    {
        N operator()(N val) { return val->next; }
    };
    using NE = list_node *;
    using CE = const list_node *;

  public:
    using const_iterator = base_bidirectional_iterator<CE, value_fn<CE, const E *>, prev_fn<CE>, next_fn<CE>>;
    using iterator = base_bidirectional_iterator<NE, value_fn<NE, E *>, prev_fn<NE>, next_fn<NE>>;

    struct list_info_node;

    linked_list(Allocator *allocator)
        : allocator_(allocator)
        , head_(allocator_->New<list_info_node>())
        , tail_(allocator_->New<list_info_node>())
        , count_(0)
    {
        head_->next = (list_node *)tail_;
        head_->prev = nullptr;
        tail_->prev = (list_node *)head_;
        tail_->next = nullptr;
    };

    linked_list(Allocator *allocator, std::initializer_list<E> il)
        : linked_list(allocator)
    {
        for (const E &a : il)
        {
            push_back(a);
        }
    };

    ~linked_list() { free(); }

    linked_list(const linked_list &rhs) { copy(rhs); }

    linked_list(linked_list &&rhs) noexcept { move(std::move(rhs)); }

    linked_list &operator=(const linked_list &rhs)
    {
        if (&rhs == this) [[unlikely]]
            return *this;
        free();
        copy(rhs);
        return *this;
    }

    linked_list &operator=(linked_list &&rhs) noexcept
    {
        if (&rhs == this) [[unlikely]]
            return *this;
        free();
        move(std::move(rhs));
        return *this;
    }

    template <typename... Args> iterator push_back(Args &&...args)
    {
        list_node *node = allocator_->New<list_node>(std::forward<Args>(args)...);
        node->next = (list_node *)tail_;
        tail_->prev->next = node;
        node->prev = tail_->prev;
        tail_->prev = node;
        count_++;
        return iterator(node);
    }

    template <typename... Args> iterator push_front(Args &&...args)
    {
        list_node *node = allocator_->New<list_node>(std::forward<Args>(args)...);
        list_node *next = ((list_node *)head_)->next;
        ((list_node *)head_)->next = node;
        node->next = next;
        node->prev = ((list_node *)head_);
        next->prev = node;
        count_++;
        return iterator(node);
    }

    E pop_back()
    {
        CXXASSERT(tail_->prev != (list_node *)head_ && count_ > 0);
        E e = tail_->prev->element;
        list_node *node = tail_->prev;
        node->prev->next = (list_node *)tail_;
        tail_->prev = node->prev;
        allocator_->Delete(node);
        count_--;
        return e;
    };

    E pop_front()
    {
        CXXASSERT(tail_->prev != (list_node *)head_ && count_ > 0);
        E e = head_->next->element;
        list_node *node = head_->next;
        node->next->prev = (list_node *)head_;
        tail_->next = node->prev;
        head_->next = head_->next->next;
        allocator_->Delete(node);
        count_--;
        return e;
    };

    size_t size() const { return count_; }

    iterator begin() const { return iterator(head_->next); }

    iterator end() const { return iterator((list_node *)tail_); }

    iterator rbegin() const { return iterator(((list_node *)tail_)->prev); }

    iterator rend() const { return iterator((list_node *)head_); }

    bool empty() const { return count_ == 0; }

    const E &back() const
    {
        CXXASSERT(tail_->prev != (list_node *)head_ && count_ > 0);
        return tail_->prev->element;
    }

    const E &front() const
    {
        CXXASSERT(tail_->prev != (list_node *)head_ && count_ > 0);
        return head_->next->element;
    }

    E &back()
    {
        CXXASSERT(tail_->prev != (list_node *)head_ && count_ > 0);
        return tail_->prev->element;
    }

    E &front()
    {
        CXXASSERT(tail_->prev != (list_node *)head_ && count_ > 0);
        return head_->next->element;
    }

    /// returns node iterator if find else returns iterator::end()
    iterator find(const E &e)
    {
        auto it = begin();
        auto last = end();
        while (it != last)
        {
            if (*it == e)
            {
                return it;
            }
            ++it;
        }
        return last;
    }

    /// insert node before parameter iter
    template <typename... Args> iterator insert(iterator iter, Args... args)
    {
        list_node *after_node = iter.get();
        list_node *node = allocator_->New<list_node>(std::forward<Args>(args)...);
        auto last = after_node->prev;

        last->next = node;
        node->next = after_node;
        node->prev = last;
        after_node->prev = node;
        count_++;
        return iterator(node);
    }

    iterator remove(iterator iter)
    {
        if (iter == end() || iter == --begin()) [[unlikely]]
            return end();

        list_node *node = iter.get();
        list_node *next = node->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        count_--;
        allocator_->Delete(node);
        return iterator(next);
    }

    void clear() noexcept
    {
        auto node = ((list_node *)head_)->next;
        while (node != (list_node *)tail_)
        {
            auto node2 = node->next;
            allocator_->Delete(node);
            node = node2;
        }
        head_->next = (list_node *)tail_;
        tail_->prev = (list_node *)head_;
        count_ = 0;
    }

    E &at(size_t idx)
    {
        CXXASSERT(idx < count_);
        size_t i = 0;
        auto it = begin();
        auto last = end();
        while (it != last && i < idx)
        {
            ++it;
            i++;
        }
        return *it;
    }

  private:
    void free() noexcept
    {
        if (head_ != nullptr)
        {
            clear();
            allocator_->Delete(head_);
            allocator_->Delete(tail_);
            head_ = nullptr;
            tail_ = nullptr;
        }
    }

    void copy(const linked_list &rhs)
    {
        count_ = 0;
        allocator_ = rhs.allocator_;
        head_ = allocator_->New<list_info_node>();
        tail_ = allocator_->New<list_info_node>();

        head_->next = (list_node *)tail_;
        head_->prev = nullptr;
        tail_->prev = (list_node *)head_;
        tail_->next = nullptr;
        auto iter = rhs.begin();
        while (iter != rhs.end())
        {
            E val = *iter;
            push_back(std::move(val));
            iter++;
        }
    }

    void move(linked_list &&rhs) noexcept
    {
        allocator_ = rhs.allocator_;
        head_ = rhs.head_;
        tail_ = rhs.tail_;
        count_ = rhs.count_;
        rhs.head_ = nullptr;
        rhs.tail_ = nullptr;
        rhs.count_ = 0;
    }

  private:
    Allocator *allocator_;
    list_info_node *head_, *tail_;
    size_t count_;

    size_t calc_size() const
    {
        size_t i = 0;
        auto iter = begin();
        while (iter != end())
        {
            i++;
            ++iter;
        }
        return i;
    }

  public:
    struct list_node
    {
        E element;
        list_node *prev, *next;

        template <typename... Args>
        list_node(Args &&...args)
            : element(std::forward<Args>(args)...)
        {
        }
    };
    struct list_info_node
    {
        char data[sizeof(list_node) - sizeof(list_node *) * 2];
        list_node *prev, *next;
        list_info_node(){};
    };

    static_assert(sizeof(list_node) == sizeof(list_info_node));
};
} // namespace freelibcxx