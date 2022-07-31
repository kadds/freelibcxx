#include "vector.hpp"
#include "common.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace freelibcxx;

TEST_CASE("empty vector", "vector")
{
    vector<int> vec(&LibAllocatorV);
    REQUIRE(vec.size() == 0);
    REQUIRE(vec.capacity() == 0);
    REQUIRE(vec.empty());
}

TEST_CASE("insert vector", "vector")
{
    SECTION("push")
    {
        vector<int> vec(&LibAllocatorV);
        vec.push_back(2);
        vec.push_front(3);
        vec.push_back(1); // 3 2 1
        REQUIRE(vec.size() == 3);
        REQUIRE(vec[0] == 3);
        REQUIRE(vec[1] == 2);
        REQUIRE(vec[2] == 1);
    }
    SECTION("insert")
    {
        vector<int> vec(&LibAllocatorV, {1, 2, 3});
        vec.insert(++vec.begin(), -1);
        vec.insert(vec.end(), -2);
        REQUIRE(vec.size() == 5);
        REQUIRE(vec[0] == 1);
        REQUIRE(vec[1] == -1);
        REQUIRE(vec[2] == 2);
        REQUIRE(vec[3] == 3);
        REQUIRE(vec[4] == -2);
    }
}

TEST_CASE("remove element in vector", "vector")
{
    vector<int> vec(&LibAllocatorV, {1, 2, 3});
    vec.remove_at(2); // 1 2
    REQUIRE(vec.size() == 2);
    REQUIRE(vec[1] == 2);
    vec.remove_at(0); // 2
    REQUIRE(vec.size() == 1);
    REQUIRE(vec[0] == 2);
    vec.remove_at(0);
    REQUIRE(vec.size() == 0);

    SECTION("range")
    {
        vector<int> vec(&LibAllocatorV, {1, 2, 3, 4, 5});
        vec.remove_at(1, 3);
        REQUIRE(vec.size() == 3);
        REQUIRE(vec[1] == 4);
    }
}

TEST_CASE("resize vector", "vector")
{
    vector<int> vec(&LibAllocatorV, {1, 2, 3});
    vec.resize(4, -1);
    REQUIRE(vec.size() == 4);
    REQUIRE(vec[3] == -1);
    vec.resize(2, 0);
    REQUIRE(vec.size() == 2);
    REQUIRE(vec[1] == 2);
}

TEST_CASE("iterator vector", "vector")
{
    SECTION("noconst")
    {
        vector<int> vec(&LibAllocatorV, {1, 2, 3});
        int j = 1;
        for (auto &i : vec)
        {
            REQUIRE(i == j);
            i = j + 1;
            j++;
        }
    }
    SECTION("const")
    {
        const vector<int> vec(&LibAllocatorV, {1, 2, 3});
        int j = 1;
        for (auto &i : vec)
        {
            REQUIRE(i == j);
            j++;
        }
    }
}

TEST_CASE("copy vector", "vector")
{
    vector<int> vec(&LibAllocatorV, {1, 2, 3});
    vector<int> vec2 = vec;
    REQUIRE(vec.size() == 3);
    REQUIRE(vec2.size() == 3);
    REQUIRE(vec2[0] == 1);

    vec2.push_back(4);
    vec.push_back(5);
    REQUIRE(vec2.size() == 4);
    REQUIRE(vec2[3] == 4);
    vec2 = vec;
    REQUIRE(vec2.size() == 4);
    REQUIRE(vec2[3] == 5);
}

TEST_CASE("expand vector", "vector")
{
    vector<int> vec(&LibAllocatorV);
    vec.ensure(10000);
    for (int i = 0; i < 10000; i++)
    {
        vec.push_back(i);
    }
    vec.truncate(400);
    REQUIRE(vec.size() == 400);
    vec.expand(402, 1);
    REQUIRE(vec.size() == 402);
    REQUIRE(vec[401] == 1);
    vec.shrink(vec.size());
    REQUIRE(vec.capacity() == 402);
}

TEST_CASE("move vector", "vector")
{
    vector<int> vec(&LibAllocatorV, {1, 2, 3});
    auto d0 = vec.data();
    vector<int> vec2 = std::move(vec);
    auto d1 = vec2.data();
    REQUIRE(vec2.size() == 3);
    REQUIRE(vec.size() == 0);
    REQUIRE(d0 == d1);
    vec2.push_back(4);

    vector<int> vec3(&LibAllocatorV, {4, 5});
    vec3 = std::move(vec2);

    REQUIRE(vec3.size() == 4);
    REQUIRE(vec3[3] == 4);
    REQUIRE(vec2.size() == 0);
}

TEST_CASE("object vector", "vector")
{
    vector<Int> vec(&LibAllocatorV, {1, 2, 3});
    vec.push_back(4, 6);
    vec.insert_at(0, 0, 0);
    vec.ensure(10);
    vec.remove_at(1);
    REQUIRE(vec[3] == Int(4, 6));
}
