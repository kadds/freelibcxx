#include "bit_set.hpp"
#include "common.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace freelibcxx;

TEST_CASE("create bitset", "bitset")
{
    bit_set s(&LibAllocatorV, 128);
    REQUIRE(s.count() == 128);
    bit_set_inplace<512> v;
    REQUIRE(v.count() == 512);
}

TEST_CASE("set/clean biset", "bitset")
{
    bit_set_inplace<1024> s;
    s.clean(2);
    for (int i = 0; i < 1023; i++)
    {
        s.set(i);
    }
    s.clean(513);

    REQUIRE(s.get(0));
    REQUIRE(s.get(2));
    REQUIRE(s.get(3));
    REQUIRE(!s.get(513));
    REQUIRE(s.get(1022));
    REQUIRE(!s.get(1023));

    s.set(513);
    REQUIRE(s.get(513));
}

TEST_CASE("set/clean all bitset", "bitset")
{
    bit_set_inplace<1024> s;
    s.set_all();
    REQUIRE(s.get(0));
    REQUIRE(s.get(1021));
    REQUIRE(s.get(64));
    REQUIRE(s.get(32));
    REQUIRE(s.get(1023));

    s.clean_all();
    REQUIRE(!s.get(0));
    REQUIRE(!s.get(1023));

    s.set_all(3, 1021);
    REQUIRE(!s.get(0));
    REQUIRE(!s.get(2));
    REQUIRE(s.get(3));
    REQUIRE(s.get(1023));

    s.clean_all(64, 1024 - 64);

    REQUIRE(s.get(63));
    REQUIRE(s.get(3));
    REQUIRE(!s.get(64));
    REQUIRE(!s.get(1019));
    REQUIRE(!s.get(1023));
}

TEST_CASE("scan bitset", "bitset")
{
    bit_set_inplace<128> s;
    s.set(8);
    s.set(9);
    REQUIRE(s.scan_zero() == 0);
    REQUIRE(s.scan_lead() == 8);
    REQUIRE(s.scan_zero(2, 8) == 2);
    REQUIRE(s.scan_lead(8, 10) == 8);
    REQUIRE(s.scan_lead(9, 10) == 9);

    s.set(0);
    REQUIRE(s.scan_zero() == 1);
    REQUIRE(s.scan_zero(1, 5) == 1);

    s.set_all(0, 64);
    REQUIRE(s.scan_zero() == 64);
    REQUIRE(s.scan_zero(31, 64) == 64);
}
