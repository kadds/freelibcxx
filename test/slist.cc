#include "common.hpp"
#include "singly_linked_list.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace freelibcxx;

TEST_CASE("create", "slist")
{
    singly_linked_list<int> list(&LibAllocatorV);
    assert(list.size() == 0);
}

TEST_CASE("push", "slist")
{
    singly_linked_list<int> list(&LibAllocatorV);
    list.push_front(1);
    list.push_front(2);
    list.push_front(3);
    assert(list.size() == 3 && list.at(0) == 3);
    assert(list.at(1) == 2);
    assert(list.at(2) == 1);
}

TEST_CASE("insert", "slist")
{
    singly_linked_list<int> list(&LibAllocatorV, {1, 2, 3});
    list.insert_after(list.begin(), std::move(-1));
    list.insert_after(++list.begin(), std::move(-2));
    assert(list.size() == 5);
    assert(list.at(0) == 1);
    assert(list.at(1) == -1);
    assert(list.at(2) == -2);
    assert(list.at(3) == 2);
    assert(list.at(4) == 3);
}

TEST_CASE("remove", "slist")
{
    singly_linked_list<int> list(&LibAllocatorV, {1, 2, 3});
    list.remove(list.begin());
    assert(list.size() == 2 && list.at(1) == 3);
    list.remove(++list.begin());
    assert(list.size() == 1 && list.at(0) == 2);
}

TEST_CASE("iterator", "slist")
{
    singly_linked_list<int> list(&LibAllocatorV, {1, 2, 3});
    int j = 1;
    for (auto i : list)
    {
        assert(i == j);
        j++;
    }
}

TEST_CASE("const_iterator", "slist")
{
    const singly_linked_list<int> list(&LibAllocatorV, {1, 2, 3});
    int j = 1;
    for (const auto &i : list)
    {
        assert(i == j);
        j++;
    }
}

TEST_CASE("empty", "slist")
{
    singly_linked_list<int> list(&LibAllocatorV);
    assert(list.empty());
    list.clear();
}

TEST_CASE("copy", "slist")
{
    singly_linked_list<int> list(&LibAllocatorV, {1, 2, 3});
    singly_linked_list<int> list2(list);
    assert(list2.size() == 3 && list.size() == 3);
    list2.push_front(std::move(4));
    list.push_front(std::move(5));
    assert(list2.size() == 4 && list2.at(0) == 4);
    list2 = list;
    assert(list2.size() == 4 && list2.at(0) == 5);
}

TEST_CASE("move", "slist")
{
    singly_linked_list<int> list(&LibAllocatorV, {1, 2, 3});
    singly_linked_list<int> list2(std::move(list));
    assert(list2.size() == 3 && list.size() == 0);
    list2.push_front(std::move(4));
    assert(list2.size() == 4 && list2.at(0) == 4);

    singly_linked_list<int> list3(&LibAllocatorV, {1, 2, 3});
    list3 = std::move(list2);

    assert(list3.size() == 4 && list3.at(0) == 4 && list2.size() == 0);
}

TEST_CASE("object", "slist")
{
    singly_linked_list<Int> list(&LibAllocatorV, {1, 2, 3});
    list.push_front(0);
    list.insert_after(go_iterator(list.begin(), 1), 2, 3);
    assert(list.at(2) == Int(2, 3));
}
