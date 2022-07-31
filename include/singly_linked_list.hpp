#pragma once
#include "allocator.hpp"
#include "assert.hpp"
#include "iterator.hpp"
#include <utility>

namespace freelibcxx
{
template <typename E> class singly_linked_list
{
  public:
    struct list_node;
    struct list_info_node;

  private:
    template <typename N, typename K> struct value_fn
    {
        K operator()(N val) { return &val->element; }
    };
    template <typename N> struct next_fn
    {
        N operator()(N val) { return val->next; }
    };

    using NE = list_node *;
    using CE = const list_node *;

  public:
    using const_iterator = base_forward_iterator<CE, value_fn<CE, const E *>, next_fn<CE>>;
    using iterator = base_forward_iterator<NE, value_fn<NE, E *>, next_fn<NE>>;

    singly_linked_list(Allocator *allocator)
        : head_(allocator->New<list_info_node>())
        , tail_(allocator->New<list_info_node>())
        , count_(0)
        , allocator_(allocator)
    {
        head_->next = (list_node *)tail_;
        tail_->next = nullptr;
    };

    singly_linked_list(Allocator *allocator, std::initializer_list<E> il)
        : singly_linked_list(allocator)
    {
        auto iter = il.begin();
        auto end = il.end();
        if (iter != end)
        {
            E val = *iter;
            push_front(std::move(val));
            iter++;
        }
        auto last_insert_iter = begin();
        while (iter != end)
        {
            E val = *iter;
            last_insert_iter = insert_after(last_insert_iter, std::move(val));
            iter++;
        }
    };

    ~singly_linked_list() { free(); }

    singly_linked_list(const singly_linked_list &rhs) { copy(rhs); }

    singly_linked_list(singly_linked_list &&rhs) { move(std::move(rhs)); }

    singly_linked_list &operator=(const singly_linked_list &rhs)
    {
        if (&rhs == this) [[unlikely]]
            return *this;

        free();
        copy(rhs);
        return *this;
    }

    singly_linked_list &operator=(singly_linked_list &&rhs)
    {

        if (&rhs == this) [[unlikely]]
            return *this;

        free();
        move(std::move(rhs));
        return *this;
    }

    template <typename... Args> iterator push_front(Args &&...args)
    {
        list_node *node = allocator_->New<list_node>(std::forward<Args>(args)...);
        node->next = head_->next;
        head_->next = node;
        count_++;
        return iterator(node);
    }

    E pop_front()
    {
        CXXASSERT(head_->next != tail_ && count_ > 0);

        E e = head_->next->element;
        list_node *node = head_->next;
        head_->next = head_->next->next;
        allocator_->Delete(node);
        count_--;
        return e;
    };

    size_t size() const { return count_; }

    iterator begin() const { return iterator(head_->next); }

    iterator end() const { return iterator((list_node *)tail_); }

    bool empty() const { return head_->next == (list_node *)tail_; }

    const E &front() const
    {
        CXXASSERT(count_ > 0);
        return head_->next->element;
    }

    E &front()
    {
        CXXASSERT(count_ > 0);
        return head_->next->element;
    }

    template <typename... Args> iterator insert_after(iterator iter, Args &&...args)
    {
        list_node *prev_node = iter.get();
        list_node *node = allocator_->New<list_node>(std::forward<Args>(args)...);

        node->next = prev_node->next;
        prev_node->next = node;

        count_++;
        return iterator(node);
    }

    iterator remove(iterator iter)
    {
        if (iter == end() || iter == iterator((list_node *)head_)) [[unlikely]]
            return end();
        list_node *pnode;
        if (iter == begin())
        {
            pnode = (list_node *)head_;
        }
        else
        {
            iterator prev = previous_iterator(begin(), iter);
            pnode = prev.get();
        }

        auto inode = iter.get();
        auto next = iter.get()->next;
        pnode->next = next;
        allocator_->Delete(inode);
        count_--;
        return iterator(next);
    }

    void clear()
    {
        auto node = ((list_node *)head_)->next;
        while (node != (list_node *)tail_)
        {
            auto node2 = node->next;
            allocator_->Delete(node);
            node = node2;
        }
        head_->next = (list_node *)tail_;
        count_ = 0;
    }

    E &at(size_t idx)
    {
        size_t i = 0;
        iterator it = begin();
        iterator last = end();
        while (it != last && i < idx)
        {
            ++it;
            i++;
        }
        return *it;
    }

  private:
    list_info_node *head_, *tail_;
    size_t count_;
    Allocator *allocator_;

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

    void free()
    {
        if (head_ != nullptr)
        {
            clear();
            allocator_->Delete(head_);
            allocator_->Delete(tail_);
            head_ = nullptr;
            tail_ = nullptr;
            count_ = 0;
        }
    }

    void copy(const singly_linked_list &rhs)
    {
        count_ = 0;
        allocator_ = rhs.allocator_;
        head_ = allocator_->New<list_info_node>();
        tail_ = allocator_->New<list_info_node>();

        head_->next = (list_node *)tail_;
        tail_->next = nullptr;

        auto iter = rhs.begin();
        auto end = rhs.end();
        if (iter != end)
        {
            E val = *iter;
            push_front(std::move(val));
            iter++;
        }
        auto last_insert_iter = begin();
        while (iter != end)
        {
            E val = *iter;
            last_insert_iter = insert_after(last_insert_iter, std::move(val));
            iter++;
        }
    }

    void move(singly_linked_list &&rhs)
    {
        allocator_ = rhs.allocator_;
        head_ = rhs.head_;
        tail_ = rhs.tail_;
        count_ = rhs.count_;
        rhs.head_ = nullptr;
        rhs.tail_ = nullptr;
        rhs.count_ = 0;
    }

  public:
    struct list_node
    {
        E element;
        list_node *next;

        template <typename... Args>
        list_node(Args &&...args)
            : element(std::forward<Args>(args)...)
        {
        }
    };
    struct list_info_node
    {
        char data[sizeof(list_node) - sizeof(list_node *)];
        list_node *next;
        list_info_node(){};
    };
    static_assert(sizeof(list_node) == sizeof(list_info_node));
};
} // namespace freelibcxx