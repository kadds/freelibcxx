#include "catch2/internal/catch_run_context.hpp"
#include "common.hpp"
#include "freelibcxx/hash_map.hpp"
#include "freelibcxx/optional.hpp"
#include "freelibcxx/string.hpp"
#include <catch2/catch_test_macros.hpp>
#include <random>
#include <unordered_map>

using namespace freelibcxx;

TEST_CASE("create hashmap", "hashmap")
{
    hash_map<int, int> map(&LibAllocatorV);
    REQUIRE(map.size() == 0);
}

TEST_CASE("insert hashmap", "hashmap")
{
    hash_map<int, int> map(&LibAllocatorV);
    map.insert(1, 1);
    map.insert(2, -1);
    map.insert(3, 5);
    map.insert(4, 6);
    REQUIRE(map.size() == 4);
    optional<int> v;
    v = map.get(1);
    REQUIRE(v.has_value());
    REQUIRE(v.value() == 1);

    v = map.get(2);
    REQUIRE(v.has_value());
    REQUIRE(v.value() == -1);

    v = map.get(3);
    REQUIRE(v.has_value());
    REQUIRE(v.value() == 5);

    v = map.get(4);
    REQUIRE(v.has_value());
    REQUIRE(v.value() == 6);

    v = map.get(6);
    REQUIRE(!v.has_value());
}

TEST_CASE("remove hashmap", "hashmap")
{
    hash_map<int, int> map(&LibAllocatorV, {{1, 2}, {3, 4}, {5, 6}});
    map.remove(1);
    REQUIRE(!map.has(1));
    map.remove(5);
    REQUIRE(!map.has(5));
    map.remove(6);
    REQUIRE(map.has(3));
}

TEST_CASE("iterator hashmap", "hashmap")
{
    hash_map<int, int> map(&LibAllocatorV, {{1, 2}, {3, 4}, {5, 6}});
    std::unordered_map<int, int> m;
    for (int i = 1; i <= 6; i += 2)
    {
        m.insert(std::make_pair(i, i + 1));
    }

    SECTION("noconst")
    {
        for (auto &i : map)
        {
            REQUIRE(m[i.key] == i.value);
            i.value++;
        }
    }
    SECTION("const")
    {
        const auto map2 = map;
        for (const auto &i : map2)
        {
            REQUIRE(m[i.key] == i.value);
        }
    }
}

TEST_CASE("copy hashmap", "hashmap")
{
    hash_map<int, int> map(&LibAllocatorV, {{1, 2}, {3, 4}, {5, 6}});
    hash_map<int, int> map2(map);
    REQUIRE(map2.size() == 3);
    REQUIRE(map.size() == 3);
    map2.insert(std::move(4), std::move(0));
    map.insert(std::move(5), std::move(0));

    REQUIRE(map2.size() == 4);
    REQUIRE(map2.has(4));
    REQUIRE(!map.has(4));
    map2 = map;
    REQUIRE(map2.size() == 4);
    REQUIRE(map2.has(5));
}

TEST_CASE("move hashmap", "hashmap")
{
    hash_map<int, int> map(&LibAllocatorV, {{1, 2}, {3, 4}, {5, 6}});
    hash_map<int, int> map2(std::move(map));
    REQUIRE(map2.size() == 3);
    REQUIRE(map.size() == 0);
    map2.insert(std::move(4), std::move(0));
    REQUIRE(map2.size() == 4);
    REQUIRE(map2.has(4));

    hash_map<int, int> map3(&LibAllocatorV, {{1, 2}});
    map3 = std::move(map2);
    REQUIRE(map3.size() == 4);
    REQUIRE(map3.has(4));
    REQUIRE(map2.size() == 0);
}

TEST_CASE("hashset", "hashmap")
{
    hash_set<int> map(&LibAllocatorV, {1, 2, 3});
    REQUIRE(map.has(1));
    REQUIRE(map.has(2));
    REQUIRE(map.has(3));
    REQUIRE(!map.has(4));
}

TEST_CASE("string key hashmap", "hashmap")
{
    hash_map<freelibcxx::string, int> map(&LibAllocatorV);
    std::unordered_map<std::string, int> m;
    std::mt19937_64 rng(Catch::rngSeed());

    for (int i = 0; i < 10; i++)
    {
        std::string ss;
        for (int j = 0; j < 100; j++)
        {
            ss += 'a' + rng() % 26;
        }

        int v = rng();
        m[ss] = v;
        map.insert(freelibcxx::string(&LibAllocatorV, ss.c_str(), ss.size()), v);
    }

    for (auto item : m)
    {
        auto val = map.get(const_string_view(item.first.c_str(), item.first.size()));
        REQUIRE(val.has_value());
        REQUIRE(val.value() == item.second);
    }
}