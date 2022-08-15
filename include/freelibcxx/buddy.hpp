#pragma once
#include "freelibcxx/algorithm.hpp"
#include "freelibcxx/assert.hpp"
#include "freelibcxx/optional.hpp"
#include <cstddef>
#include <iterator>
#include <limits>
namespace freelibcxx
{

namespace detail
{

template <typename T, typename INDEX>
concept buddy_operator = requires(T t)
{
    // set index, prev, next, order, used, merged
    t.set((INDEX)1, 0, 0, 0, false, true);

    t.set_next((INDEX)1, 0);
    t.set_prev((INDEX)1, 0);
    t.set_order((INDEX)1, 2);
    t.set_merged((INDEX)1);
    t.clear_merged((INDEX)1);
    t.set_used((INDEX)1);
    t.clear_used((INDEX)1);

    t.get_next((INDEX)1);
    t.get_prev((INDEX)1);
    t.get_order((INDEX)1);
    t.get_merged((INDEX)1);
    t.get_used((INDEX)1);

    // get tuple [prev, next, order, used]
    t.get(0);
};

} // namespace detail

template <typename OPERATOR, int MAXORDER = 11, typename INDEX = size_t>
requires detail::buddy_operator<OPERATOR, INDEX>
class buddy
{
    static_assert(MAXORDER < 20, "order too large");

    using index_t = INDEX;
    using count_t = index_t;
    constexpr static int max_order = MAXORDER;
    constexpr static int max_pages = 1 << MAXORDER;

  public:
    buddy(size_t pages, OPERATOR oper);
    buddy(const buddy &) = delete;
    buddy &operator=(const buddy &) = delete;

    void alloc_at(index_t index, count_t count = 1);

    optional<index_t> alloc(count_t pages);
    void free(index_t page_index);

    count_t debug_free_pages();
    count_t free_pages() const { return free_pages_; }
    count_t total_pages() const { return pages_; }

  private:
    // return last page index
    void init_orders(index_t beg, index_t end, int order);

    void merge(int order, size_t max_merge_times_for_each_of_level = std::numeric_limits<size_t>::max());
    void lazy_merge(int order) { merge(order, 1); }

    index_t split(index_t index);

    void detach_page_from_list(index_t index);
    void attach_page_to_list(index_t index, int order);

    inline index_t empty_index() const { return pages_; }

    index_t bro_page_index(index_t page);
    index_t bro_page_index(index_t page, int order);

    constexpr static inline INDEX order2pages(int order) { return ((INDEX)1) << order; }

  private:
    OPERATOR oper_;

    index_t pages_;
    index_t free_pages_;
    index_t orders_[MAXORDER + 1];
};

template <typename OPERATOR, int MAXORDER, typename INDEX>
requires detail::buddy_operator<OPERATOR, INDEX> buddy<OPERATOR, MAXORDER, INDEX>::buddy(size_t pages, OPERATOR oper)
    : oper_(oper)
    , pages_(pages)
    , free_pages_(pages)
{
    for (auto &o : orders_)
    {
        o = empty_index();
    }
    size_t big_pages = pages / max_pages * max_pages;

    init_orders(0, big_pages, max_order);
    init_orders(big_pages, pages, 0);

    merge(0);
}

template <typename OPERATOR, int MAXORDER, typename INDEX>
requires detail::buddy_operator<OPERATOR, INDEX>
void buddy<OPERATOR, MAXORDER, INDEX>::init_orders(index_t beg, index_t end, int order)
{
    if (beg >= end) [[unlikely]]
    {
        return;
    }
    auto cur = beg;

    // fill merged flags
    while (cur < end)
    {
        oper_.set(cur, empty_index(), empty_index(), order, false, true);
        cur++;
    }

    auto pages = order2pages(order);
    cur = beg;
    while (cur < end)
    {
        oper_.set(cur, cur - pages, cur + pages, order, false, false);
        cur += pages;
    }
    auto last = orders_[order];
    oper_.set_next(end - pages, last);
    oper_.set_prev(beg, empty_index());
    if (last != empty_index())
    {
        oper_.set_prev(last, end - pages);
    }
    orders_[order] = beg;
}

template <typename OPERATOR, int MAXORDER, typename INDEX>
requires detail::buddy_operator<OPERATOR, INDEX>
void buddy<OPERATOR, MAXORDER, INDEX>::merge(int from_order, size_t max_merge_times_for_each_of_level)
{
    for (int o = from_order; o < max_order; o++)
    {
        size_t a = max_merge_times_for_each_of_level;
        auto cur = orders_[o];
        while (cur != empty_index() && a-- > 0)
        {
            auto index = cur;
            auto bro = bro_page_index(index);
            const auto [prev, next, order, used, merged] = oper_.get(index);
            cur = next;

            if (used) [[unlikely]]
            {
                continue;
            }

            if (bro == empty_index()) [[unlikely]]
            {
                continue;
            }

            const auto [bro_prev, bro_next, bro_order, bro_used, bro_merged] = oper_.get(bro);
            if (bro_used) [[unlikely]]
            {
                continue;
            }
            if (bro_order != order)
            {
                // wait bro block merge
                continue;
            }

            CXXASSERT(order == o);

            if (cur == bro)
            {
                cur = bro_next;
            }

            // can merge
            detach_page_from_list(index);
            detach_page_from_list(bro);
            if (bro < index)
            {
                std::swap(bro, index);
            }
            attach_page_to_list(index, o + 1);
            oper_.clear_merged(index);

            oper_.set_order(bro, o + 1);
            oper_.set_merged(bro);
        }
    }
}

template <typename OPERATOR, int MAXORDER, typename INDEX>
requires detail::buddy_operator<OPERATOR, INDEX> INDEX buddy<OPERATOR, MAXORDER, INDEX>::debug_free_pages()
{
    INDEX pages = 0;
    for (int order = 0; order <= max_order; order++)
    {
        auto cur = orders_[order];
        INDEX n = 0;
        while (cur != empty_index())
        {
            cur = oper_.get_next(cur);
            n++;
        }
        pages += n * order2pages(order);
    }
    return pages;
}

template <typename OPERATOR, int MAXORDER, typename INDEX>
requires detail::buddy_operator<OPERATOR, INDEX> INDEX buddy<OPERATOR, MAXORDER, INDEX>::bro_page_index(index_t index)
{
    const auto order = oper_.get_order(index);
    return bro_page_index(index, order);
}

template <typename OPERATOR, int MAXORDER, typename INDEX>
requires detail::buddy_operator<OPERATOR, INDEX> INDEX buddy<OPERATOR, MAXORDER, INDEX>::bro_page_index(index_t index,
                                                                                                        int order)
{
    const auto pages = order2pages(order);
    if ((index / pages) % 2 == 0)
    {
        // right is bro
        if (index + pages * 2 <= pages_)
        {
            return index + pages;
        }
        return empty_index();
    }
    else
    {
        CXXASSERT(index >= pages);
        // left is bro
        return index - pages;
    }
}

template <typename OPERATOR, int MAXORDER, typename INDEX>
requires detail::buddy_operator<OPERATOR, INDEX>
void buddy<OPERATOR, MAXORDER, INDEX>::attach_page_to_list(index_t index, int order)
{
    auto head = orders_[order];
    oper_.set(index, empty_index(), head, order, false, false);
    if (head != empty_index())
    {
        oper_.set_prev(head, index);
    }
    orders_[order] = index;
}

template <typename OPERATOR, int MAXORDER, typename INDEX>
requires detail::buddy_operator<OPERATOR, INDEX>
void buddy<OPERATOR, MAXORDER, INDEX>::detach_page_from_list(index_t index)
{
    const auto [prev, next, order, used, merged] = oper_.get(index);
    auto head = orders_[order];
    if (prev != empty_index())
    {
        oper_.set_next(prev, next);
    }
    if (next != empty_index())
    {
        oper_.set_prev(next, prev);
    }

    if (head == index)
    {
        orders_[order] = next;
    }
    oper_.set_prev(index, empty_index());
    oper_.set_next(index, empty_index());
}

template <typename OPERATOR, int MAXORDER, typename INDEX>
requires detail::buddy_operator<OPERATOR, INDEX> INDEX buddy<OPERATOR, MAXORDER, INDEX>::split(index_t index)
{
    const auto [prev, next, order, used, merged] = oper_.get(index);
    CXXASSERT(order > 0 && !used && !merged);

    detach_page_from_list(index);

    auto next_order = order - 1;
    auto bro = bro_page_index(index, next_order);
    auto head = orders_[next_order];
    index_t p;

    if (bro < pages_)
    {
        const auto [bro_prev, bro_next, bro_order, bro_used, bro_merged] = oper_.get(bro);
        CXXASSERT(!bro_used && bro_merged);
        if (bro < index)
        {
            oper_.set(bro, empty_index(), index, next_order, false, false);
            oper_.set(index, bro, head, next_order, false, false);
            p = index;
        }
        else
        {
            oper_.set(index, empty_index(), bro, next_order, false, false);
            oper_.set(bro, index, head, next_order, false, false);
            p = bro;
        }
    }
    else
    {
        // no brother
        oper_.set(index, empty_index(), head, next_order, false, false);
        p = index;
    }
    if (head != empty_index())
    {
        oper_.set_prev(head, p);
    }

    orders_[next_order] = index;

    return index;
}

template <typename OPERATOR, int MAXORDER, typename INDEX>
requires detail::buddy_operator<OPERATOR, INDEX> optional<INDEX> buddy<OPERATOR, MAXORDER, INDEX>::alloc(count_t pages)
{
    pages = next_pow_of_2(pages);
    const int order = __builtin_ffsl(pages) - 1;

    CXXASSERT(order <= max_order);

    int split_top_order = order;
    while (split_top_order <= max_order && orders_[split_top_order] == empty_index())
    {
        split_top_order++;
    }
    if (split_top_order > max_order)
    {
        // no free pages
        return nullopt;
    }

    // split from top order

    for (int o = split_top_order; o > order && o > 0; o--)
    {
        split(orders_[o]);
    }

    // finally we load pages from orders_[order]

    auto index = orders_[order];
    CXXASSERT(index != empty_index());

    // mark status
    detach_page_from_list(index);
    oper_.set_used(index);

    CXXASSERT(free_pages_ >= pages);
    free_pages_ -= pages;
    return index;
}

template <typename OPERATOR, int MAXORDER, typename INDEX>
requires detail::buddy_operator<OPERATOR, INDEX>
void buddy<OPERATOR, MAXORDER, INDEX>::free(index_t page_index)
{
    const auto [prev, next, order, used, merged] = oper_.get(page_index);
    CXXASSERT(used && !merged);

    attach_page_to_list(page_index, order);

    lazy_merge(order);

    free_pages_ += order2pages(order);
    CXXASSERT(free_pages_ <= pages_);
}

template <typename OPERATOR, int MAXORDER, typename INDEX>
requires detail::buddy_operator<OPERATOR, INDEX>
void buddy<OPERATOR, MAXORDER, INDEX>::alloc_at(index_t index, count_t count)
{
    // todo: mark pages [index, index+count) used
}

} // namespace freelibcxx
