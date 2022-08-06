#include "common.hpp"
#include "freelibcxx/singly_linked_list.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace freelibcxx;

TEST_CASE("create slist", "slist")
{
    singly_linked_list<int> list(&LibAllocatorV);
    REQUIRE(list.size() == 0);
    REQUIRE(list.empty());
}

TEST_CASE("insert slist", "slist")
{
    SECTION("push")
    {
        singly_linked_list<int> list(&LibAllocatorV);
        list.push_front(1);
        list.push_front(2);
        list.push_front(3);
        REQUIRE(list.size() == 3);
        REQUIRE(list.at(0) == 3);
        REQUIRE(list.at(1) == 2);
        REQUIRE(list.at(2) == 1);
        list.clear();
        REQUIRE(list.size() == 0);
    }
    SECTION("insert")
    {
        singly_linked_list<int> list(&LibAllocatorV, {1, 2, 3});
        list.insert_after(list.begin(), std::move(-1));
        list.insert_after(++list.begin(), std::move(-2));
        REQUIRE(list.size() == 5);
        REQUIRE(list.at(0) == 1);
        REQUIRE(list.at(1) == -1);
        REQUIRE(list.at(2) == -2);
        REQUIRE(list.at(3) == 2);
        REQUIRE(list.at(4) == 3);
    }
}

TEST_CASE("remove element from slist", "slist")
{
    singly_linked_list<int> list(&LibAllocatorV, {1, 2, 3});
    list.remove(list.begin());
    REQUIRE(list.size() == 2);
    REQUIRE(list.at(1) == 3);
    list.remove(++list.begin());
    REQUIRE(list.size() == 1);
    REQUIRE(list.at(0) == 2);
}

TEST_CASE("iterator slist", "slist")
{
    SECTION("noconst")
    {
        singly_linked_list<int> list(&LibAllocatorV, {1, 2, 3});
        int j = 1;
        for (auto i : list)
        {
            REQUIRE(i == j);
            j++;
        }
    }
    SECTION("const")
    {
        const singly_linked_list<int> list(&LibAllocatorV, {1, 2, 3});
        int j = 1;
        for (const auto &i : list)
        {
            REQUIRE(i == j);
            j++;
        }
    }
}

TEST_CASE("copy slist", "slist")
{
    singly_linked_list<int> list(&LibAllocatorV, {1, 2, 3});
    singly_linked_list<int> list2(list);
    REQUIRE(list2.size() == 3);
    REQUIRE(list.size() == 3);
    list2.push_front(std::move(4));
    list.push_front(std::move(5));
    REQUIRE(list2.size() == 4);
    REQUIRE(list2.at(0) == 4);
    list2 = list;
    REQUIRE(list2.size() == 4);
    REQUIRE(list2.at(0) == 5);
}

TEST_CASE("move slist", "slist")
{
    singly_linked_list<int> list(&LibAllocatorV, {1, 2, 3});
    singly_linked_list<int> list2(std::move(list));
    REQUIRE(list2.size() == 3);
    REQUIRE(list.size() == 0);
    list2.push_front(std::move(4));
    REQUIRE(list2.size() == 4);
    REQUIRE(list2.at(0) == 4);

    singly_linked_list<int> list3(&LibAllocatorV, {1, 2, 3});
    list3 = std::move(list2);

    REQUIRE(list3.size() == 4);
    REQUIRE(list3.at(0) == 4);
    REQUIRE(list2.size() == 0);
}

TEST_CASE("object slist", "slist")
{
    singly_linked_list<Int> list(&LibAllocatorV, {1, 2, 3});
    list.push_front(0);
    list.insert_after(go_iterator(list.begin(), 1), 2, 3);
    REQUIRE(list.at(2) == Int(2, 3));
}
