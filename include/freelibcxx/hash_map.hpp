#pragma once
#include "freelibcxx/algorithm.hpp"
#include "freelibcxx/allocator.hpp"
#include "freelibcxx/hash.hpp"
#include "freelibcxx/iterator.hpp"
#include "freelibcxx/optional.hpp"
#include "freelibcxx/utils.hpp"
#include <type_traits>
#include <utility>

namespace freelibcxx
{

template <typename K> struct hash_set_pair
{
    using Key = K;
    K key;
    template <typename... Args>
    hash_set_pair(Args &&...args)
        : key(std::forward<Args>(args)...)
    {
    }
};

template <typename K, typename V> struct hash_map_pair : hash_set_pair<K>
{
    V value;
    template <typename... Args>
    hash_map_pair(K key, Args &&...args)
        : hash_set_pair<K>(key)
        , value(std::forward<Args>(args)...)
    {
    }
};

template <typename T>
concept hash_map_has_value = requires(T t)
{
    t.value;
};

template <typename P, typename hash_func> class base_hash_map
{
  public:
    struct node_t;
    struct entry;

  protected:
    template <typename CE, typename NE> struct base_holder
    {
        CE table;
        CE end_table;
        NE node;
        base_holder(CE table, CE end_table, NE node)
            : table(table)
            , end_table(end_table)
            , node(node)
        {
        }
        bool operator==(const base_holder &rhs) const
        {
            return table == rhs.table && end_table == rhs.end_table && node == rhs.node;
        }
        bool operator!=(const base_holder &rhs) const { return !operator==(rhs); }
    };

    template <typename H, typename E> struct value_fn
    {
        E operator()(H val) { return &val.node->content; }
    };
    template <typename H> struct next_fn
    {
        H operator()(H val)
        {
            H holder = val;
            if (!holder.node->next)
            {
                holder.node = holder.node->next;
                while (!holder.node)
                {
                    holder.table++;
                    if (holder.table >= holder.end_table)
                        break;
                    holder.node = holder.table->next;
                }
            }
            else
            {
                holder.node = holder.node->next;
            }
            return holder;
        }
    };

    using holder = base_holder<entry *, node_t *>;
    using const_holder = base_holder<const entry *, const node_t *>;
    using K = typename P::Key;

  public:
    using const_iterator =
        base_forward_iterator<const_holder, value_fn<const_holder, const P *>, next_fn<const_holder>>;
    using iterator = base_forward_iterator<holder, value_fn<holder, P *>, next_fn<holder>>;

    base_hash_map(Allocator *allocator, size_t capacity)
        : size_(0)
        , table_(nullptr)
        , cap_(0)
        , allocator_(allocator)
    {
    }

    base_hash_map(Allocator *allocator, std::initializer_list<P> il)
        : base_hash_map(allocator, il.size())
    {
        for (auto e : il)
        {
            insert(std::move(e));
        }
    }

    explicit base_hash_map(Allocator *allocator)
        : base_hash_map(allocator, 0)
    {
    }

    ~base_hash_map() { free(); }

    base_hash_map(const base_hash_map &rhs) { copy(rhs); }

    base_hash_map(base_hash_map &&rhs) { move(std::move(rhs)); }

    base_hash_map &operator=(const base_hash_map &rhs)
    {
        if (&rhs == this)
            return *this;

        free();
        copy(rhs);
        return *this;
    }

    base_hash_map &operator=(base_hash_map &&rhs)
    {
        if (&rhs == this)
            return *this;

        free();
        move(std::move(rhs));
        return *this;
    }

    template <typename... Args> iterator insert(Args &&...args)
    {
        ensure(size_ + 1);
        node_t *node = allocator_->New<node_t>(nullptr, std::forward<Args>(args)...);
        size_t hash = hash_key(node->content.key);
        node->next = table_[hash].next;
        table_[hash].next = node;
        size_++;
        return iterator(holder(&table_[hash], table_ + cap_, table_[hash].next));
    }

    bool has(const K &key)
    {
        int count = key_count(key);
        return count > 0;
    }

    int key_count(const K &key)
    {
        if (size_ == 0) [[unlikely]]
        {
            return 0;
        }
        size_t hash = hash_key(key);
        int i = 0;
        for (auto it = table_[hash].next; it != nullptr; it = it->next)
        {
            if (it->content.key == key)
                i++;
        }
        return i;
    }

    void remove(const K &key)
    {
        if (size_ == 0) [[unlikely]]
        {
            return;
        }
        size_t hash = hash_key(key);
        node_t *prev = nullptr;
        for (auto it = table_[hash].next; it != nullptr;)
        {
            if (it->content.key == key)
            {
                auto cur_node = it;
                it = it->next;

                if (prev) [[likely]]
                    prev->next = it;
                else
                    table_[hash].next = it;
                allocator_->Delete<>(cur_node);
                return;
            }
            prev = it;
            it = it->next;
        }
    }

    size_t size() const { return size_; }

    void clear()
    {
        for (size_t i = 0; i < cap_; i++)
        {
            for (auto it = table_[i].next; it != nullptr;)
            {
                auto n = it;
                it = it->next;
                allocator_->Delete<>(n);
            }
            table_[i].next = nullptr;
        }
        size_ = 0;
    }

    size_t capacity() const { return cap_; }

    iterator begin() const
    {
        auto table = this->table_;
        if (table == nullptr || size_ == 0)
        {
            return end();
        }
        auto end = this->table_ + cap_;
        auto node = this->table_->next;
        if (!node)
        {
            while (!table->next && table != end)
            {
                table++;
            }
            node = table->next;
        }
        return iterator(holder(table, end, node));
    }

    iterator end() const { return iterator(holder(table_ + cap_, table_ + cap_, nullptr)); }

  protected:
    size_t size_;
    entry *table_;
    size_t cap_;
    Allocator *allocator_;

    void recapacity(size_t new_capacity)
    {
        if (new_capacity == cap_) [[unlikely]]
        {
            return;
        }
        auto new_table = allocator_->NewArray<entry>(new_capacity);
        if (table_ != nullptr)
        {
            for (size_t i = 0; i < cap_; i++)
            {
                for (auto it = table_[i].next; it != nullptr;)
                {
                    size_t hash = hash_func()(it->content.key) % new_capacity;
                    auto next_it = it->next;
                    auto next_node = new_table[hash].next;
                    new_table[hash].next = it;
                    it->next = next_node;
                    it = next_it;
                }
            }
            allocator_->DeleteArray(cap_, table_);
        }
        table_ = new_table;
        cap_ = new_capacity;
    }

    size_t hash_key(const K &key) { return hash_func()(key) % cap_; }

    void ensure(size_t new_count)
    {
        if (new_count >= cap_ * 75 / 100)
        {
            recapacity(select_capacity(max(cap_ + 1, new_count)));
        }
    }

    void free()
    {
        if (table_ != nullptr)
        {
            clear();
            allocator_->DeleteArray(cap_, table_);
            table_ = nullptr;
        }
    }

    void copy(const base_hash_map &rhs)
    {
        allocator_ = rhs.allocator_;
        cap_ = select_capacity(rhs.size_);
        size_ = 0;
        table_ = allocator_->NewArray<entry>(cap_);
        auto iter = rhs.begin();
        while (iter != rhs.end())
        {
            auto v = *iter;
            insert(std::move(v));
            iter++;
        }
    }

    void move(base_hash_map &&rhs)
    {
        allocator_ = rhs.allocator_;
        table_ = rhs.table_;
        size_ = rhs.size_;
        cap_ = rhs.cap_;
        rhs.table_ = nullptr;
        rhs.size_ = 0;
        rhs.cap_ = 0;
    }

  public:
    struct node_t
    {
        P content;
        node_t *next;
        P &operator*() { return content; }

        template <typename... Args>
        node_t(node_t *next, Args &&...args)
            : content(std::forward<Args>(args)...)
            , next(next){};
    };

    struct entry
    {
        node_t *next;
        entry()
            : next(nullptr)
        {
        }
    };
};

template <typename K, typename V, typename hash_func = hasher<K>>
class hash_map : public base_hash_map<hash_map_pair<K, V>, hash_func>
{
  private:
    using Parent = base_hash_map<hash_map_pair<K, V>, hash_func>;
    struct key_find_func
    {
        const K &key;
        key_find_func(const K &key)
            : key(key)
        {
        }
        bool operator()(const hash_map_pair<K, V> &p) const { return p.key == key; }
    };

  public:
    using Parent::Parent;

    optional<V> get(const K &key)
    {
        if (this->size_ == 0) [[unlikely]]
        {
            return nullopt;
        }
        size_t hash = this->hash_key(key);
        for (auto it = this->table_[hash].next; it != nullptr; it = it->next)
        {
            if (it->content.key == key)
            {
                return it->content.value;
            }
        }
        return nullopt;
    }

    // danger!!!
    optional<V &> get_ref(const K &key)
    {
        if (this->count == 0) [[unlikely]]
        {
            return nullopt;
        }
        size_t hash = this->hash_key(key);
        for (auto it = this->table[hash].next; it != nullptr; it = it->next)
        {
            if (it->content.key == key)
            {
                return it->content.value;
            }
        }
        return nullopt;
    }
};

template <typename K, typename hash_func = hasher<K>> class hash_set : public base_hash_map<hash_set_pair<K>, hash_func>
{
  private:
    using Parent = base_hash_map<hash_set_pair<K>, hash_func>;

  public:
    using Parent::Parent;
};

} // namespace freelibcxx