#pragma once
#include "freelibcxx/allocator.hpp"
#include "freelibcxx/iterator.hpp"
#include "freelibcxx/random.hpp"
#include <cstddef>
#include <functional>
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
        K operator()(N val) { return &val->element_; }
    };
    template <typename N> struct prev_fn
    {
        N operator()(N val) { return val->back_; }
    };
    template <typename N> struct next_fn
    {
        N operator()(N val) { return val->level_[0].next_; }
    };

    using NE = list_node *;
    using CE = const list_node *;

  public:
    using const_iterator = base_bidirectional_iterator<CE, value_fn<CE, const E *>, prev_fn<CE>, next_fn<CE>>;
    using iterator = base_bidirectional_iterator<NE, value_fn<NE, E *>, prev_fn<CE>, next_fn<NE>>;
    /// return -1 walk left
    /// 1 walk right
    /// 0 stop
    typedef int (*each_func)(const E &element, size_t user_data);

    skip_list(Allocator *allocator, uint64_t seed = 0)
        : count_(0)
        , level_(0)
        , engine_(seed)
        , allocator_(allocator)
    {
        init();
    }

    skip_list(Allocator *allocator, uint64_t seed, std::initializer_list<E> il)
        : skip_list(allocator, seed)
    {
        for (const E &e : il)
        {
            insert(e);
        }
    }

    skip_list(const skip_list &rhs)
        : engine_(rhs.engine_.pick())
    {
        copy(rhs);
    }

    skip_list(skip_list &&rhs) noexcept
        : engine_(rhs.engine_())
    {
        move(std::move(rhs));
    }

    ~skip_list() { free(); }

    skip_list &operator=(const skip_list &rhs)
    {
        if (this == &rhs)
            return *this;
        free();
        copy(rhs);
        return *this;
    }

    skip_list &operator=(skip_list &&rhs) noexcept
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
        int cur_level = level_;

        if (node_level > cur_level)
        {
            cur_level = node_level;
        }
        int level = cur_level;

        auto node = node_;
        auto insert_node = make_node(node_level + 1, std::forward<Args>(args)...);
        node_t *cache_nodes[MAXLEVEL];
        // for each level of node
        // find the element path
        while (cur_level >= 0)
        {
            auto next = node->level_[cur_level].next_;
            while (next && next->element_ < insert_node->element_)
            {
                node = next;
                next = node->level_[cur_level].next_;
            }
            cache_nodes[cur_level] = node;
            cur_level--;
        }

        // set index node linked list
        for (int l = 0; l <= node_level; l++)
        {
            insert_node->level_[l].next_ = cache_nodes[l]->level_[l].next_;
            cache_nodes[l]->level_[l].next_ = insert_node;
        }
        insert_node->back_ = node;

        count_++;
        level_ = level;
        return iterator(insert_node);
    }

    bool remove(const E &element)
    {
        int cur_level = level_;
        auto node = node_;
        node_t *cache_nodes[MAXLEVEL];
        // for each level of node
        // find the element path
        while (cur_level >= 0)
        {
            auto next = node->level_[cur_level].next_;
            while (next && next->element_ < element)
            {
                node = next;
                next = node->level_[cur_level].next_;
            }
            cache_nodes[cur_level] = node;
            cur_level--;
        }
        auto next = node->level_[0].next_;
        if (next == nullptr || next->element_ != element)
        {
            return false;
        }

        // remote nodes each level
        for (int l = level_; l >= 0; l--)
        {
            auto cur = cache_nodes[l]->level_[l].next_;
            if (cur != nullptr) {
                cache_nodes[l]->level_[l].next_ = cur->level_[l].next_;
            }
        }
        next->element_.~E();
        allocator_->Delete(next);

        count_--;
        return true;
    }

    iterator remove(iterator iter)
    {
        auto next = iter;
        next++;
        bool ok = remove(iter.get()->element_);
        if (ok)
        {
            return next;
        }
        return iter;
    }

    size_t size() const { return count_; }

    bool empty() const { return count_ == 0; }

    size_t deep() const { return level_; }

    const E &front() const { return node_->level_[0].next_->element_; }

    iterator begin() const { return iterator(node_->level_[0].next_); }

    iterator end() const { return iterator(nullptr); }

    iterator find(const E &element) const
    {
        node_t *node = node_;
        int cur_level = level_;

        while (cur_level >= 0)
        {
            auto next = node->level_[cur_level].next_;
            while (next && next->element_ < element)
            {
                node = next;
                next = node->level_[cur_level].next_;
            }
            cur_level--;
        }
        auto next = node->level_[0].next_;
        if (next && next->element_ == element)
        {
            return iterator(next);
        }
        return iterator(nullptr);
    }

    iterator lower_find(const E &element) const
    {
        node_t *node = node_;
        int cur_level = level_;

        while (cur_level >= 0)
        {
            auto next = node->level_[cur_level].next_;
            while (next && next->element_ < element)
            {
                node = next;
                next = node->level_[cur_level].next_;
            }
            cur_level--;
        }
        if (node->element_ == element)
        {
            return iterator(node);
        }
        else
        {
            return iterator(node->level_[0].next_);
        }
    }

    iterator upper_find(const E &element) const
    {
        node_t *node = node_;
        int cur_level = level_;

        while (cur_level >= 0)
        {
            auto next = node->level_[cur_level].next_;
            while (next && next->element_ <= element)
            {
                node = next;
                next = node->level_[cur_level].next_;
            }
            cur_level--;
        }
        if (node != nullptr)
        {
            return iterator(node->level_[0].next_);
        }
        return end();
    }

    bool has(const E &element) const { return find(element) != end(); }

    void clear() noexcept
    {
        node_t *node = node_;
        node_t *c = node->level_[0].next_;
        for (int i = MAXLEVEL - 1; i >= 0; i--)
        {
            node_->level_[i].next_ = nullptr;
        }
        while (c != nullptr)
        {
            auto next = c->level_[0].next_;
            c->element_.~E();
            allocator_->Delete(c);
            c = next;
        }
        count_ = 0;
    }

  private:
    size_t count_;
    int level_;

    node_t *node_;
    RDENG engine_;
    Allocator *allocator_;

    int rand()
    {
        int node_level = 1;
        random_generator rng(engine_);

        while (rng.gen_range(0, 2) == 0 && node_level < MAXLEVEL)
            node_level++;
        return node_level - 1;
    }

    void free() noexcept
    {
        if (node_ != nullptr)
        {
            clear();
            count_ = 0;
            level_ = 0;
            allocator_->Delete(node_);
            node_ = nullptr;
        }
    }

    void init() { node_ = make_empty_node(MAXLEVEL); }

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

    void move(skip_list &&rhs) noexcept
    {
        count_ = rhs.count_;
        level_ = rhs.level_;
        node_ = rhs.node_;
        allocator_ = rhs.allocator_;

        rhs.count_ = 0;
        rhs.level_ = 0;
        rhs.node_ = nullptr;
    }

    node_t *make_empty_node(int level)
    {
        void *n = allocator_->allocate(sizeof(node_t) + level * sizeof(index_node_t), alignof(node_t));
        node_t *node = new (n) node_t();
        for (int i = 0; i < level; i++)
        {
            node->level_[i].next_ = nullptr;
        }
        return node;
    }

    template <typename... Args> node_t *make_node(int level, Args &&...args)
    {
        void *n = allocator_->allocate(sizeof(node_t) + level * sizeof(index_node_t), alignof(node_t));
        node_t *node = new (n) node_t(nullptr, std::forward<Args>(args)...);
        return node;
    }

  public:
    struct index_node_t
    {
        node_t *next_;
    };
    struct node_t
    {
        node_t *back_;
        union
        {
            E element_;
        };
        index_node_t level_[0];
        template <typename... Args>
        node_t(node_t *back, Args &&...args)
            : back_(back)
            , element_(std::forward<Args>(args)...)
        {
        }
        node_t()
            : back_(nullptr)
        {
        }
    };
}; // namespace util
} // namespace freelibcxx