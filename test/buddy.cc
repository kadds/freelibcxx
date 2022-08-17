#include "freelibcxx/buddy.hpp"
#include "catch2/internal/catch_run_context.hpp"
#include "common.hpp"
#include "freelibcxx/random.hpp"
#include "freelibcxx/tuple.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <iostream>
#include <random>
#include <unordered_map>
#include <unordered_set>

using namespace freelibcxx;
struct Operator
{
    struct page_t
    {
        size_t prev_ = 0;
        size_t next_ = 0;
        int order_ = 0;
        bool used_ = false;
        bool merged_ = false;
    };

  public:
    Operator(size_t n) { vec_.resize(n); }
    void set(size_t index, size_t prev, size_t next, int order, bool used, bool merged)
    {
        vec_[index] = page_t{prev, next, order, used, merged};
    }
    void set_next(size_t index, size_t next) { vec_[index].next_ = next; }
    void set_prev(size_t index, size_t prev) { vec_[index].prev_ = prev; }
    void set_order(size_t index, int order) { vec_[index].order_ = order; }
    void set_merged(size_t index) { vec_[index].merged_ = true; }
    void clear_merged(size_t index) { vec_[index].merged_ = false; }

    void set_used(size_t index) { vec_[index].used_ = true; }
    void clear_used(size_t index) { vec_[index].used_ = false; }

    size_t get_next(size_t index) const { return vec_[index].next_; }
    size_t get_prev(size_t index) const { return vec_[index].prev_; }
    int get_order(size_t index) const { return vec_[index].order_; }

    bool get_merged(size_t index) const { return vec_[index].merged_; }
    bool get_used(size_t index) const { return vec_[index].used_; }

    tuple<size_t, size_t, int, bool, bool> get(size_t index)
    {
        auto &p = vec_[index];
        return make_tuple(p.prev_, p.next_, p.order_, p.used_, p.merged_);
    }

    std::vector<page_t> vec_;
};

TEST_CASE("buddy basic", "buddy")
{
    constexpr int test_pages = 14401;
    Operator oper(test_pages);
    buddy<Operator> buddy(test_pages, oper);
    REQUIRE(buddy.debug_free_pages() == buddy.free_pages());

    auto index0 = buddy.alloc(4).value();
    REQUIRE(buddy.debug_free_pages() == test_pages - 4);

    auto index1 = buddy.alloc(31).value();
    REQUIRE(buddy.debug_free_pages() == test_pages - 4 - 32);

    buddy.free(index0);
    REQUIRE(buddy.debug_free_pages() == test_pages - 32);

    buddy.free(index1);
    REQUIRE(buddy.debug_free_pages() == test_pages);
}

TEST_CASE("buddy alloc simple", "buddy")
{
    auto fn = [](int test_pages) {
        Operator oper(test_pages);
        buddy<Operator> buddy(test_pages, oper);
        REQUIRE(buddy.debug_free_pages() == buddy.free_pages());

        for (int i = 0; i < test_pages; i++)
        {
            if (!buddy.alloc(1).has_value())
            {
                REQUIRE(-1 == i);
            }
        }
        REQUIRE(buddy.debug_free_pages() == 0);
        REQUIRE(buddy.free_pages() == 0);

        // alloc fail
        REQUIRE(!buddy.alloc(1).has_value());

        for (int i = 0; i < test_pages; i++)
        {
            buddy.free(i);
        }
        REQUIRE(buddy.debug_free_pages() == test_pages);
        REQUIRE(buddy.free_pages() == test_pages);
    };
    fn(5);
    fn((1 << 11));
    fn((1 << 18) + 124);
}

TEST_CASE("buddy random alloc", "buddy")
{
    constexpr int test_pages = 1 << 20;
    Operator oper(test_pages);
    buddy<Operator> buddy(test_pages, oper);

    std::mt19937 mt(Catch::rngSeed());

    std::unordered_map<size_t, size_t> set;

    for (int i = 0; i < 100000; i++)
    {
        if ((mt() % 2) == 0 || set.size() == 0)
        {
            int alloc_pages = 1 << (mt() % 10);

            auto index = buddy.alloc(alloc_pages);
            if (index.has_value())
            {
                set.emplace(index.value(), alloc_pages);
                continue;
            }
        }
        auto beg = set.begin();
        buddy.free(beg->first);
        set.erase(beg);
    }

    auto pages = 0;
    for (auto [index, page] : set)
    {
        pages += page;
    }

    auto free = test_pages - pages;

    REQUIRE(free == buddy.free_pages());
    REQUIRE(free == buddy.debug_free_pages());
}

TEST_CASE("buddy alloc at", "buddy")
{
    constexpr int test_pages = 15746;
    int pages = test_pages;
    Operator oper(test_pages);
    buddy<Operator> buddy(test_pages, oper);

    buddy.alloc_at(2, 5);
    REQUIRE(!buddy.alloc_at(4, 2));

    pages -= 5;
    REQUIRE(buddy.debug_free_pages() == pages);
    buddy.free(2);
    buddy.free(4);
    buddy.free(6);
    pages += 5;

    REQUIRE(buddy.debug_free_pages() == pages);

    buddy.alloc_at(1024, 1024 * 2);
    pages -= 1024 * 2;
    REQUIRE(buddy.debug_free_pages() == pages);

    buddy.alloc_at(1023, 1);
    pages -= 1;
    REQUIRE(buddy.debug_free_pages() == pages);
    REQUIRE(buddy.free_pages() == pages);

    buddy.alloc_at(0, 16);
    pages -= 16;
    REQUIRE(buddy.debug_free_pages() == pages);
    REQUIRE(buddy.free_pages() == pages);

    for (int i = 0; i < pages; i++)
    {
        auto index = buddy.alloc(1);
        if (!index.has_value())
        {
            REQUIRE(-1 == i);
        }
        auto idx = index.value();
        if (idx >= 1023 && idx < 1024 * 2 + 1024)
        {
            REQUIRE(index.value() == -1);
        }
        if (idx >= 0 && idx < 6)
        {
            REQUIRE(index.value() == -1);
        }
    }
    REQUIRE(buddy.debug_free_pages() == 0);
}
