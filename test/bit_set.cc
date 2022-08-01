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
    s.reset_bit(2);
    for (int i = 0; i < 1023; i++)
    {
        s.set_bit(i);
    }
    s.reset_bit(513);

    REQUIRE(s[0]);
    REQUIRE(s[2]);
    REQUIRE(s[3]);
    REQUIRE(!s[513]);
    REQUIRE(s[1022]);
    REQUIRE(!s[1023]);

    s.set_bit(513);
    REQUIRE(s[513]);
}

TEST_CASE("set/clean all bitset", "bitset")
{
    bit_set_inplace<1024> s;
    s.set_all();
    REQUIRE(s[0]);
    REQUIRE(s[1021]);
    REQUIRE(s[64]);
    REQUIRE(s[32]);
    REQUIRE(s[1023]);

    s.reset_all();
    REQUIRE(!s[0]);
    REQUIRE(!s[1023]);

    s.set_all(3, 1021);
    REQUIRE(!s[0]);
    REQUIRE(!s[2]);
    REQUIRE(s[3]);
    REQUIRE(s[1023]);

    s.reset_all(64, 1024 - 64);

    REQUIRE(s[63]);
    REQUIRE(s[3]);
    REQUIRE(!s[64]);
    REQUIRE(!s[1019]);
    REQUIRE(!s[1023]);
}

TEST_CASE("scan bitset", "bitset")
{
    bit_set_inplace<128> s;
    s.set_bit(8);
    s.set_bit(9);
    REQUIRE(s.scan_zero() == 0);
    REQUIRE(s.scan_lead() == 8);
    REQUIRE(s.scan_zero(2, 8) == 2);
    REQUIRE(s.scan_lead(8, 10) == 8);
    REQUIRE(s.scan_lead(9, 10) == 9);

    s.set_bit(0);
    REQUIRE(s.scan_zero() == 1);
    REQUIRE(s.scan_zero(1, 5) == 1);

    s.set_all(0, 64);
    REQUIRE(s.scan_zero() == 64);
    REQUIRE(s.scan_zero(31, 64) == 64);
}
