#include "skip_list.hpp"
#include "catch2/internal/catch_run_context.hpp"
#include "common.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace freelibcxx;

TEST_CASE("create skip list", "skip_list")
{
    skip_list<int> list(&LibAllocatorV);
    REQUIRE(list.size() == 0);
}

TEST_CASE("insert skip list", "skip_list")
{
    skip_list<int> list(&LibAllocatorV, Catch::rngSeed());
    list.insert(1);
    list.insert(2);

    REQUIRE(list.size() == 2);
    REQUIRE(list.has(1));
    REQUIRE(list.has(2));
}

TEST_CASE("iterator skip list", "skip_list")
{
    SECTION("noconst")
    {
        skip_list<int> list(&LibAllocatorV, Catch::rngSeed(), {1, 2, 3, 4, 5, 6});
        int j = 0;
        for (auto i : list)
        {
            REQUIRE(i == j);
            j++;
        }
    }
    SECTION("const")
    {
        const skip_list<int> list(&LibAllocatorV, Catch::rngSeed(), {1, 2, 3, 4, 5, 6});
        int j = 1;
        for (const auto &i : list)
        {
            REQUIRE(i == j);
            j++;
        }
    }
}

TEST_CASE("copy skip list", "skip_list")
{
    skip_list<int> list(&LibAllocatorV, Catch::rngSeed(), {1, 2, 3});
    skip_list<int> list2 = list;
    REQUIRE(list2.size() == 3);
    REQUIRE(list2.has(1));
    REQUIRE(list2.has(2));
    REQUIRE(list2.has(3));

    REQUIRE(list.size() == 3);

    list2.insert(4);
    list.insert(5);

    REQUIRE(list2.size() == 4);
    REQUIRE(list.size() == 4);
    REQUIRE(list2.has(4));
    list2 = list;

    REQUIRE(list2.size() == 4);
    REQUIRE(list2.has(5));
}

TEST_CASE("move skip list", "skip_list")
{
    skip_list<int> list(&LibAllocatorV, Catch::rngSeed(), {1, 2, 3});
    skip_list<int> list2 = std::move(list);
    REQUIRE(list2.size() == 3);
    REQUIRE(list2.has(1));
    REQUIRE(list2.has(2));
    REQUIRE(list2.has(3));

    REQUIRE(list.size() == 0);

    list2.insert(4);

    REQUIRE(list2.size() == 4);
    REQUIRE(list2.has(4));
    skip_list<int> list3(&LibAllocatorV, Catch::rngSeed(), {4, 5});
    list3 = std::move(list2);
    REQUIRE(list3.size() == 4);
    REQUIRE(list3.has(4));
    REQUIRE(list2.size() == 0);
}

TEST_CASE("find element in skip list", "skip_list")
{
    skip_list<int> list(&LibAllocatorV, Catch::rngSeed(), {1, 2, 4});
    auto iter = list.find(2);
    REQUIRE(iter.get()->element == 2);
    iter = list.find(3);
    REQUIRE(iter == list.end());
    iter = list.find(4);
    REQUIRE(iter.get()->element == 4);
}

TEST_CASE("find binary in skip list", "skip_list")
{
    skip_list<int> list(&LibAllocatorV, Catch::rngSeed(), {1, 2, 4});
    auto iter = list.upper_find(2);
    REQUIRE(iter.get()->element == 2);

    iter = list.lower_find(2);
    REQUIRE(iter.get()->element == 4);

    iter = list.upper_find(3);
    REQUIRE(iter.get()->element == 4);
    iter = list.upper_find(4);
    REQUIRE(iter.get()->element == 4);
}
