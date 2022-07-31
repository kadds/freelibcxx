#include "common.hpp"
#include "linked_list.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace freelibcxx;

TEST_CASE("create list", "list")
{
    linked_list<int> list(&LibAllocatorV);
    REQUIRE(list.size() == 0);
    REQUIRE(list.empty());
}

TEST_CASE("insert list", "list")
{
    SECTION("push")
    {

        linked_list<int> list(&LibAllocatorV);
        list.push_back(2);
        list.push_front(3);
        list.push_back(1);

        REQUIRE(list.size() == 3);
        REQUIRE(list.at(0) == 3);
        REQUIRE(list.at(1) == 2);
        REQUIRE(list.at(2) == 1);
        list.clear();
        REQUIRE(list.size() == 0);
    }
    SECTION("insert")
    {

        linked_list<int> list(&LibAllocatorV, {1, 2, 3});
        list.insert(++list.begin(), std::move(-1));
        list.insert(list.end(), std::move(-2));

        REQUIRE(list.size() == 5);
        REQUIRE(list.at(0) == 1);
        REQUIRE(list.at(1) == -1);
        REQUIRE(list.at(2) == 2);
        REQUIRE(list.at(3) == 3);
        REQUIRE(list.at(4) == -2);
    }
}

TEST_CASE("remove list", "list")
{
    linked_list<int> list(&LibAllocatorV, {1, 2, 3});
    list.remove(list.begin());
    REQUIRE(list.size() == 2);
    REQUIRE(list.at(1) == 3);

    list.remove(++list.begin());
    REQUIRE(list.size() == 1);
    REQUIRE(list.at(0) == 2);
}

TEST_CASE("iterator list", "list")
{
    SECTION("noconst")
    {

        linked_list<int> list(&LibAllocatorV, {1, 2, 3});
        int j = 1;
        for (auto i : list)
        {
            REQUIRE(i == j);
            j++;
        }
    }
    SECTION("const")
    {
        const linked_list<int> list(&LibAllocatorV, {1, 2, 3});
        int j = 1;
        for (const auto &i : list)
        {
            REQUIRE(i == j);
            j++;
        }
    }
}

TEST_CASE("copy list", "list")
{
    linked_list<int> list(&LibAllocatorV, {1, 2, 3});
    linked_list<int> list2(list);
    REQUIRE(list2.size() == 3);
    REQUIRE(list.size() == 3);
    list2.push_back(std::move(4));
    list.push_back(std::move(5));

    REQUIRE(list2.size() == 4);
    REQUIRE(list2.at(3) == 4);
    list2 = list;
    REQUIRE(list2.size() == 4);
    REQUIRE(list2.at(3) == 5);
}

TEST_CASE("move list", "list")
{
    linked_list<int> list(&LibAllocatorV, {1, 2, 3});
    linked_list<int> list2(std::move(list));
    REQUIRE(list2.size() == 3);
    REQUIRE(list.size() == 0);

    list2.push_back(std::move(4));
    REQUIRE(list2.size() == 4);
    REQUIRE(list2.at(3) == 4);

    linked_list<int> list3(&LibAllocatorV, {1, 2, 3});
    list3 = std::move(list2);

    REQUIRE(list3.size() == 4);
    REQUIRE(list3.at(3) == 4);
    REQUIRE(list2.size() == 0);
}

TEST_CASE("object list", "list")
{
    linked_list<Int> list(&LibAllocatorV, {1, 2, 3});
    list.push_back(4);
    list.push_front(0);
    list.insert(go_iterator(list.begin(), 1), 2, 3);
    REQUIRE(list.at(1) == Int(2, 3));
}
