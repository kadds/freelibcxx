#include "freelibcxx/skip_list.hpp"
#include "catch2/internal/catch_run_context.hpp"
#include "common.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace freelibcxx;

struct mutable_key_entry
{
    explicit mutable_key_entry(int *key)
        : key(key)
    {
    }

    bool operator<(const mutable_key_entry &other) const { return *key < *other.key; }
    bool operator==(const mutable_key_entry &other) const { return key == other.key; }

    int *key;
};

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
        int j = 1;
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
    SECTION("remove")
    {
        skip_list<int> list(&LibAllocatorV, Catch::rngSeed(), {1, 2, 3, 4, 5, 6});
        auto it = list.find(4);
        REQUIRE(list.remove(it) == list.find(5));
    }
}

TEST_CASE("remove iterator after its ordering key changes", "skip_list")
{
    int first_key = 10;
    int target_key = 20;
    int last_key = 30;
    skip_list<mutable_key_entry, mt19937_random_engine, 1> list(&LibAllocatorV, 1);
    list.insert(&first_key);
    list.insert(&target_key);
    list.insert(&last_key);

    auto target = list.begin();
    ++target;
    auto expected_next = target;
    ++expected_next;

    *target->key = 0;
    const auto next = list.remove(target);

    REQUIRE(next == expected_next);
    REQUIRE(list.size() == 2);
    for (const auto &entry : list)
    {
        REQUIRE(entry.key != &target_key);
    }
}

TEST_CASE("remove mutable-key iterator from multiple skip-list levels", "skip_list")
{
    constexpr int entry_count = 32;
    int keys[entry_count];
    skip_list<mutable_key_entry> list(&LibAllocatorV, 4);
    for (int index = 0; index < entry_count; ++index)
    {
        keys[index] = index;
        list.insert(&keys[index]);
    }

    auto target = list.begin();
    for (int index = 0; index < entry_count / 2; ++index)
        ++target;
    int *removed_key = target->key;
    auto expected_next = target;
    ++expected_next;

    *removed_key = -1;
    const auto next = list.remove(target);

    REQUIRE(next == expected_next);
    REQUIRE(list.size() == entry_count - 1);
    REQUIRE(list.find(mutable_key_entry(removed_key)) == list.end());
    for (int index = 0; index < entry_count; ++index)
    {
        if (&keys[index] != removed_key)
            REQUIRE(list.find(mutable_key_entry(&keys[index])) != list.end());
    }
    int visited = 0;
    for (const auto &entry : list)
    {
        REQUIRE(entry.key != removed_key);
        ++visited;
    }
    REQUIRE(visited == entry_count - 1);
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
    REQUIRE(*iter == 2);
    iter = list.find(3);
    REQUIRE(iter == list.end());
    iter = list.find(4);
    REQUIRE(*iter == 4);
}

TEST_CASE("find binary in skip list", "skip_list")
{
    skip_list<int> list(&LibAllocatorV, Catch::rngSeed(), {1, 2, 4});
    auto iter = list.upper_find(2);
    REQUIRE(iter != list.end());
    REQUIRE(*iter == 4);

    iter = list.lower_find(2);
    REQUIRE(iter != list.end());
    REQUIRE(*iter == 2);

    iter = list.upper_find(3);
    REQUIRE(iter != list.end());
    REQUIRE(*iter == 4);

    iter = list.upper_find(4);
    REQUIRE(iter == list.end());

    iter = list.upper_find(10);
    REQUIRE(iter == list.end());

    iter = list.lower_find(5);
    REQUIRE(iter == list.end());
}
