#pragma once
#include "allocator.hpp"
#include "common.hpp"
#include "iterator.hpp"
#include "random.hpp"
#include <functional>
#include <linux/limits.h>
#include <utility>

namespace freelibcxx
{
template <typename E, typename RDENG = mt19937_random_engine, int MAXLEVEL = 20> class skip_list
{
  public:
    struct node_t;
    using list_node = node_t;

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
    /// return -1 walk left
    /// 1 walk right
    /// 0 stop
    typedef int (*each_func)(const E &element, size_t user_data);

    skip_list(Allocator *allocator, uint64_t seed = 0)
        : count_(0)
        , level_(1)
        , engine_(allocator->New<RDENG>(seed))
        , allocator_(allocator)
    {
        init();
    }

    skip_list(Allocator *allocator, uint64_t seed, std::initializer_list<E> il)
        : skip_list(allocator, seed)
    {
        for (auto e : il)
        {
            insert(std::move(e));
        }
    }

    skip_list(const skip_list &rhs) { copy(rhs); }

    skip_list(skip_list &&rhs) { move(std::move(rhs)); }

    ~skip_list() { free(); }

    skip_list &operator=(const skip_list &rhs)
    {
        if (this == &rhs)
            return *this;
        free();
        copy(rhs);
        return *this;
    }

    skip_list &operator=(skip_list &&rhs)
    {
        if (this == &rhs)
            return *this;
        free();
        move(std::move(rhs));
        return *this;
    }

    template <typename... Args> iterator insert(Args &&...args)
    {
        int node_level = this->rand();

        if (node_level > level_)
            level_ = node_level;

        auto new_end_node = allocator_->New<node_t>(nullptr, std::forward<Args>(args)...);
        // for each level of node
        // find the element path
        auto node = stack_[MAXLEVEL - level_];
        for (int i = 0; i < level_; i++)
        {
            while (node->next && node->next->end_node->element < new_end_node->element)
            {
                node = node->next;
            }
            stack_[i] = node;
            node = node->child;
        }

        // create end node
        auto end_node_prev = stack_[level_ - 1];
        new_end_node->next = end_node_prev->next;
        end_node_prev->next = new_end_node;
        count_++;

        // set index node linked list
        int base = level_ - node_level;
        for (int i = node_level - 1; i > 0; i--)
        {
            auto next = stack_[base + i - 1]->next;
            auto child = stack_[base + i]->next;
            stack_[base + i - 1]->next = allocator_->New<node_t>();
            stack_[base + i - 1]->next->set(next, child, new_end_node);
        }

        return iterator(new_end_node);
    }

    iterator remove(const E &element)
    {
        auto node = stack_[MAXLEVEL - level_];
        int lev_delete = 0;
        int cur_level = 0;
        // for each get element per level
        for (int i = 0; i < level_; i++)
        {
            while (node->next && node->next->end_node->element < element)
            {
                node = node->next;
            }
            if (node->next && node->next->end_node->element == element)
            {
                auto next = node->next->next;
                allocator_->Delete(node->next);
                node->next = next;
                cur_level++;
                if (i < level_ - 1 && !stack_[i + MAXLEVEL - level_]->next)
                    lev_delete++;
            }
            stack_[i] = node;
            node = node->child;
        }
        if (cur_level == 0)
            return iterator(nullptr);

        if (cur_level == level_)
            level_ -= lev_delete;

        count_--;
        return iterator(stack_[cur_level - 1]->next);
    }

    iterator remove(iterator iter) { return remove(*iter); }

    size_t size() const { return count_; }

    bool empty() const { return count_ == 0; }

    size_t deep() const { return level_; }

    E front() const { return stack_[MAXLEVEL - 1]->next->element; }

    iterator begin() const { return iterator(stack_[MAXLEVEL - 1]->next); }

    iterator end() const { return iterator(nullptr); }

    iterator find(const E &element) const
    {
        node_t *node = stack_[MAXLEVEL - level_], *last_node = nullptr;

        for (int i = 0; i < level_; i++)
        {
            while (node->next && node->next->end_node->element < element)
            {
                node = node->next;
            }
            last_node = node;
            node = node->child;
        }
        if (last_node->next && last_node->next->end_node->element == element)
            return iterator(last_node->next);
        return iterator(nullptr);
    }

    iterator lower_find(const E &element) const { return lower_find(element, std::greater<E>()); }

    template <typename CMP = std::greater<E>> iterator lower_find(const E &element, CMP cmp) const
    {
        node_t *node = stack_[MAXLEVEL - level_], *last_node = nullptr;

        for (int i = 0; i < level_; i++)
        {
            while (node->next && !cmp(node->next->end_node->element, element))
            {
                node = node->next;
            }
            last_node = node;
            node = node->child;
        }
        return iterator(last_node);
    }

    iterator upper_find(const E &element) const { return upper_find(element, std::greater_equal<E>()); }

    template <typename CMP = std::greater_equal<E>> iterator upper_find(const E &element, CMP cmp) const
    {
        node_t *node = stack_[MAXLEVEL - level_], *last_node = nullptr;

        for (int i = 0; i < level_; i++)
        {
            while (node->next && !cmp(node->next->end_node->element, element))
            {
                node = node->next;
            }
            last_node = node;
            node = node->child;
        }
        return iterator(last_node);
    }

    bool has(const E &element) const { return find(element) != end(); }

    void clear()
    {
        count_ = 0;
        for (int i = 0; i < level_; i++)
        {
            node_t *p0 = stack_[MAXLEVEL - i - 1];
            node_t *node = p0->next;
            while (node)
            {
                auto next = node->next;
                allocator_->Delete<node_t>(node);
                node = next;
            }
            p0->next = nullptr;
        }
    }

  private:
    size_t count_;
    int level_;

    node_t *stack_[MAXLEVEL];
    /// an array likes  [
    ///  node_level 1
    ///  node_level 2
    ///  node_level 3
    ///  node_level 4 root
    /// ]
    RDENG *engine_;
    Allocator *allocator_;

    int rand()
    {
        int node_level = 1;
        random_generator rng(*engine_);

        while (rng.gen_range(0, 2) == 0 && node_level < MAXLEVEL)
            node_level++;
        return node_level;
    }

    void free()
    {
        if (stack_ != nullptr)
        {
            clear();
            count_ = 0;
            level_ = 0;
            for (int i = 0; i < MAXLEVEL; i++)
            {
                allocator_->Delete<node_t>(stack_[MAXLEVEL - i - 1]);
            }
        }
    }

    void init()
    {
        node_t *last = nullptr;
        for (int i = 0; i < MAXLEVEL; i++)
        {
            stack_[MAXLEVEL - i - 1] = allocator_->New<node_t>();
            stack_[MAXLEVEL - i - 1]->set(nullptr, last, nullptr);
            last = stack_[MAXLEVEL - i - 1];
        }
    }

    void copy(const skip_list &rhs)
    {
        count_ = 0;
        level_ = 1;
        allocator_ = rhs.allocator_;
        init();
        // TODO: n*log(n)
        auto iter = rhs.begin();
        auto end = rhs.end();
        while (iter != end)
        {
            E v = *iter;
            insert(std::move(v));
            iter++;
        }
    }

    void move(skip_list &&rhs)
    {
        count_ = rhs.count_;
        level_ = rhs.level_;
        memcpy(stack_, rhs.stack_, sizeof(stack_));
        memset(rhs.stack_, 0, sizeof(rhs.stack_));
        allocator_ = rhs.allocator_;
        rhs.count_ = 0;
        rhs.level_ = 0;
    }

  public:
    struct node_t
    {
        node_t *next;
        node_t *end_node;
        union
        {
            node_t *child;
            E element;
        };

        node_t() {}

        void set(node_t *next, node_t *child, node_t *end_node)
        {
            this->next = next;
            this->child = child;
            this->end_node = end_node;
        }

        template <typename... Args>
        node_t(node_t *next, Args &&...args)
            : next(next)
            , end_node(this)
            , element(std::forward<Args>(args)...)
        {
        }
    };

}; // namespace util
} // namespace freelibcxx