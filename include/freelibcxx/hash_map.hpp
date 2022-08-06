#pragma once
#include "freelibcxx/allocator.hpp"
#include "freelibcxx/hash.hpp"
#include "freelibcxx/iterator.hpp"
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
        : count(0)
        , table(nullptr)
        , allocator(allocator)
    {
        cap = select_capacity(capacity);
        if (cap > 0)
        {
            table = allocator->NewArray<entry>(cap);
        }
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
        check_capacity(count + 1);
        node_t *node = allocator->New<node_t>(nullptr, std::forward<Args>(args)...);
        size_t hash = hash_key(node->content.key);
        node->next = table[hash].next;
        table[hash].next = node;
        count++;
        return iterator(holder(&table[hash], table + cap, table[hash].next));
    }

    bool has(const K &key)
    {
        int count = key_count(key);
        return count > 0;
    }

    int key_count(const K &key)
    {
        if (count == 0) [[unlikely]]
        {
            return 0;
        }
        size_t hash = hash_key(key);
        int i = 0;
        for (auto it = table[hash].next; it != nullptr; it = it->next)
        {
            if (it->content.key == key)
                i++;
        }
        return i;
    }

    void remove(const K &key)
    {
        if (count == 0) [[unlikely]]
        {
            return;
        }
        size_t hash = hash_key(key);
        node_t *prev = nullptr;
        for (auto it = table[hash].next; it != nullptr;)
        {
            if (it->content.key == key)
            {
                auto cur_node = it;
                it = it->next;

                if (prev) [[likely]]
                    prev->next = it;
                else
                    table[hash].next = it;
                allocator->Delete<>(cur_node);
                check_capacity(count--);
                return;
            }
            prev = it;
            it = it->next;
        }
    }

    size_t size() const { return count; }

    void clear()
    {
        for (size_t i = 0; i < cap; i++)
        {
            for (auto it = table[i].next; it != nullptr;)
            {
                auto n = it;
                it = it->next;
                allocator->Delete<>(n);
            }
            table[i].next = nullptr;
        }
        count = 0;
    }

    size_t capacity() const { return cap; }

    iterator begin() const
    {
        auto table = this->table;
        if (table == nullptr || count == 0)
        {
            return end();
        }
        auto node = this->table->next;
        if (!node)
        {
            while (!table->next && table != (this->table + cap))
            {
                table++;
            }
            node = table->next;
        }
        return iterator(holder(table, this->table + cap, node));
    }

    iterator end() const { return iterator(holder(table + cap, table + cap, nullptr)); }

  protected:
    size_t count;
    entry *table;
    // fix point 12345.67
    size_t cap;
    Allocator *allocator;

    void recapacity(size_t new_capacity)
    {
        if (new_capacity == cap) [[unlikely]]
        {
            return;
        }
        auto new_table = allocator->NewArray<entry>(new_capacity);
        for (size_t i = 0; i < cap; i++)
        {
            for (auto it = table[i].next; it != nullptr;)
            {
                size_t hash = hash_func()(it->content.key) % new_capacity;
                auto next_it = it->next;
                auto next_node = new_table[hash].next;
                new_table[hash].next = it;
                it->next = next_node;
                it = next_it;
            }
        }
        if (table != nullptr)
        {
            allocator->DeleteArray(cap, table);
        }
        table = new_table;
        cap = new_capacity;
    }

    size_t select_capacity(size_t capacity) { return capacity; }

    size_t hash_key(const K &key) { return hash_func()(key) % cap; }

    void check_capacity(size_t new_count)
    {
        if (cap == 0 && new_count > 0)
        {
            recapacity(2);
        }
        else if (new_count >= cap * 75 / 100)
        {
            recapacity(cap * 2);
        }
    }

    void free()
    {
        if (table != nullptr)
        {
            clear();
            allocator->DeleteArray(cap, table);
            table = nullptr;
        }
    }

    void copy(const base_hash_map &rhs)
    {
        allocator = rhs.allocator;
        cap = select_capacity(rhs.count);
        count = 0;
        table = allocator->NewArray<entry>(cap);
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
        allocator = rhs.allocator;
        table = rhs.table;
        count = rhs.count;
        cap = rhs.cap;
        rhs.table = nullptr;
        rhs.count = 0;
        rhs.cap = 0;
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
    bool get(const K &key, V &v)
    {
        if (this->count == 0) [[unlikely]]
        {
            return 0;
        }
        size_t hash = this->hash_key(key);
        for (auto it = this->table[hash].next; it != nullptr; it = it->next)
        {
            if (it->content.key == key)
            {
                v = it->content.value;
                return true;
            }
        }
        return false;
    }
    V *get_ptr(const K &key)
    {
        if (this->count == 0) [[unlikely]]
        {
            return 0;
        }
        size_t hash = this->hash_key(key);
        for (auto it = this->table[hash].next; it != nullptr; it = it->next)
        {
            if (it->content.key == key)
            {
                return &it->content.value;
            }
        }
        return nullptr;
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