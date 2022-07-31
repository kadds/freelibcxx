#include "string.hpp"
#include "common.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace freelibcxx;

TEST_CASE("empty string", "string")
{
    string ss(&LibAllocatorV);
    REQUIRE(ss.size() == 0);
    REQUIRE(strcmp(ss.data(), "") == 0);
    REQUIRE(ss == "");
}

TEST_CASE("sso string", "string")
{
    string ss(&LibAllocatorV, "hi");
    REQUIRE(ss.size() == 2);
    REQUIRE(strcmp("hi", ss.data()) == 0);

    // readonly string
    string ss2 = string::from_cstr("hi");
    REQUIRE(ss2.size() == 2);
    REQUIRE(strcmp("hi", ss2.cdata()) == 0);
}

TEST_CASE("append to string", "string")
{
    string ss(&LibAllocatorV);
    ss += "base1234567890";
    REQUIRE(ss.size() == 14);
    ss += "abcdefghi";
    REQUIRE(ss.size() == 23);
    ss += "jkl";
    REQUIRE(ss.size() == 26);
    REQUIRE(ss == "base1234567890abcdefghijkl");
    string ss2(&LibAllocatorV, "base");
    ss2 += "123098";
    REQUIRE(ss2 == "base123098");
}

TEST_CASE("copy string", "string")
{
    // no shared copy
    string ss(&LibAllocatorV, "hi");
    string ss2 = ss;
    REQUIRE(ss == ss2);
    ss2 += "tt";
    string ss3 = ss2;
    REQUIRE(ss3 == "hitt");
    REQUIRE(ss2 == ss3);
    REQUIRE(ss != ss2);
}

TEST_CASE("move string", "string")
{
    string ss = string(&LibAllocatorV, "hi");
    string ss2 = std::move(ss);
    REQUIRE(ss2 == "hi");
    REQUIRE(ss == "");
}

TEST_CASE("remove char from string", "string")
{
    string ss(&LibAllocatorV, "abcdefghi0123456789");
    ss += "01234567";
    ss.remove_at(1, 3);
    REQUIRE(ss == "adefghi012345678901234567");
    ss.remove_at(18, 23);
    REQUIRE(ss == "adefghi0123456789067");
    REQUIRE(ss.pop_back() == '7');
    REQUIRE(ss == "adefghi012345678906");
    REQUIRE(ss.pop_front() == 'a');
    REQUIRE(ss == "defghi012345678906");
}

TEST_CASE("iterator of string", "string")
{
    const char *ch = "abcdefg";
    SECTION("noconst")
    {
        string ss(&LibAllocatorV, ch);
        int j = 0;
        for (auto &i : ss)
        {
            REQUIRE(i == ch[j]);
            j++;
        }
    }
    SECTION("const")
    {
        const string ss(&LibAllocatorV, ch);
        int j = 0;
        for (const auto &i : ss)
        {
            REQUIRE(i == ch[j]);
            j++;
        }
    }
    const char *ch2 = "abcdefghijklmnopqrstuvw";
    SECTION("nosso")
    {
        const string ss(&LibAllocatorV, ch2);
        int j = 0;
        for (const auto &i : ss)
        {
            REQUIRE(i == ch2[j]);
            j++;
        }
    }
}
