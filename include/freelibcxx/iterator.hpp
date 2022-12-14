#pragma once
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace freelibcxx
{
template <typename T, typename E>
concept for_each_container = requires(T t)
{
    t.begin();
    t.end();
};

template <typename T>
concept forward_iterator = requires(T t)
{
    ++t;
};

template <typename T>
concept bidirectional_iterator = requires(T t)
{
    ++t;
    --t;
};

template <typename T>
concept random_access_iterator = requires(T t)
{
    ++t;
    --t;
    t[0];
};

template <typename T>
concept must_forward_iterator = forward_iterator<T> && !bidirectional_iterator<T>;

template <typename T, typename C>
concept check_fn = requires(T t, C c)
{
    t(c);
};

template <typename T, typename C>
concept check_random_fn = requires(T t, C c)
{
    t.offset_of(c);
    t[-1];
    t[1];
};

template <typename C, typename Val, typename Next>
requires check_fn<Val, C> && check_fn<Next, C>
class base_forward_iterator
{
  private:
    C current;

  public:
    explicit base_forward_iterator(C c)
        : current(c)
    {
    }

    template <typename CV>
    requires std::is_pointer_v<CV>
    explicit base_forward_iterator(C c)
        : current(c)
    {
    }

  public:
    C &get() { return current; }

    auto &operator*() { return *Val()(current); }
    auto operator&() { return Val()(current); }
    auto operator->() { return Val()(current); }

    base_forward_iterator operator++(int)
    {
        auto old = *this;
        current = Next()(current);
        return old;
    }
    base_forward_iterator &operator++()
    {
        current = Next()(current);
        return *this;
    }
    base_forward_iterator next()
    {
        auto s = *this;
        s.current = Next()(current);
        return s;
    }

    bool operator==(const base_forward_iterator &it) const { return current == it.current; }

    bool operator!=(const base_forward_iterator &it) const { return !operator==(it); }
};

template <typename C, typename Val, typename Prev, typename Next>
requires check_fn<Val, C> && check_fn<Prev, C> && check_fn<Next, C>
class base_bidirectional_iterator
{
  private:
    C current;

  public:
    explicit base_bidirectional_iterator(C c)
        : current(c)
    {
    }

    template <typename CV>
    requires std::is_pointer_v<CV>
    explicit base_bidirectional_iterator(C c)
        : current(c)
    {
    }

  public:
    C &get() { return current; }

    auto &operator*() { return *Val()(current); }
    auto operator&() { return Val()(current); }
    auto operator->() { return Val()(current); }
    base_bidirectional_iterator operator--(int)
    {
        auto old = *this;
        current = Prev()(current);
        return old;
    }
    base_bidirectional_iterator &operator--()
    {
        current = Prev()(current);
        return *this;
    }

    base_bidirectional_iterator prev()
    {
        auto s = *this;
        s.current = Prev()(current);
        return s;
    }

    base_bidirectional_iterator operator++(int)
    {
        auto old = *this;
        current = Next()(current);
        return old;
    }
    base_bidirectional_iterator &operator++()
    {
        current = Next()(current);
        return *this;
    }
    base_bidirectional_iterator next()
    {
        auto s = *this;
        s.current = Next()(current);
        return s;
    }

    bool operator==(const base_bidirectional_iterator &it) const { return current == it.current; }

    bool operator!=(const base_bidirectional_iterator &it) const { return !operator==(it); }
};

template <typename C, typename Val, typename Random>
requires check_fn<Val, C> && check_random_fn<Random, C>
class base_random_access_iterator
{
  private:
    C current;

  public:
    explicit base_random_access_iterator(C c)
        : current(c)
    {
    }

    template <typename CV>
    requires std::is_pointer_v<CV>
    explicit base_random_access_iterator(C c)
        : current(c)
    {
    }

  public:
    C &get() { return current; }

    auto &operator*() { return *Val()(current); }
    auto operator&() { return Val()(current); }
    auto operator->() { return Val()(current); }
    base_random_access_iterator operator--(int)
    {
        auto old = *this;
        Random r(current);
        current = r[-1];
        return old;
    }
    base_random_access_iterator &operator--()
    {
        Random r(current);
        current = r[-1];
        return *this;
    }

    base_random_access_iterator prev()
    {
        Random r(current);
        auto s = base_random_access_iterator(r[-1]);
        return s;
    }

    base_random_access_iterator operator++(int)
    {
        auto old = *this;
        Random r(current);
        current = r[1];
        return old;
    }
    base_random_access_iterator &operator++()
    {
        Random r(current);
        current = r[1];
        return *this;
    }
    base_random_access_iterator next()
    {
        Random r(current);
        auto s = base_random_access_iterator(r[1]);
        return s;
    }

    bool operator==(const base_random_access_iterator &it) const { return current == it.current; }

    bool operator!=(const base_random_access_iterator &it) const { return !operator==(it); }

    base_random_access_iterator operator+(ptrdiff_t index)
    {
        Random r(current);
        return base_random_access_iterator(r[index]);
    }
    base_random_access_iterator operator-(ptrdiff_t index)
    {
        Random r(current);
        return base_random_access_iterator(r[-index]);
    }

    base_random_access_iterator &operator+=(ptrdiff_t index)
    {
        *this = operator+(index);
        return *this;
    }

    base_random_access_iterator &operator-=(ptrdiff_t index)
    {
        *this = operator-(index);
        return *this;
    }

    ptrdiff_t operator-(base_random_access_iterator to)
    {
        Random r(current);
        return r.offset_of(to.current);
    }

    C &operator[](ptrdiff_t index)
    {
        Random r(current);
        return r[index];
    }
};

template <typename T>
requires must_forward_iterator<T> T previous_iterator(T beg, T target)
{
    CXXASSERT(beg != target);
    T prev = beg;
    while (beg != target)
    {
        prev = beg;
        beg++;
    }
    return prev;
}

template <typename T>
requires forward_iterator<T> T previous_iterator(T beg, T target)
{
    CXXASSERT(beg != target);
    return --target;
}

template <typename T>
requires forward_iterator<T> T next_iterator(T end, T target)
{
    CXXASSERT(end != target);
    return ++target;
}

template <typename T>
requires forward_iterator<T> T go_iterator(T begin, size_t count)
{
    for (int i = 0; i < count; i++)
    {
        begin++;
    }
    return begin;
}

} // namespace freelibcxx