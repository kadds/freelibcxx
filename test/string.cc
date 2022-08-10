#include "freelibcxx/string.hpp"
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
    ss2 += '1';
    REQUIRE(ss2 == "base1230981");

    ss2.insert_at(2, "u", 1);
    REQUIRE(ss2 == "bause1230981");

    ss2.insert_at(12, "uv", 2);
    REQUIRE(ss2 == "bause1230981uv");
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

TEST_CASE("create string_view from string", "string")
{
    const char *test = "tri";
    string ss(&LibAllocatorV, "string");
    auto view = ss.view().substr(1, 3);
    SECTION("iterator")
    {
        int j = 0;
        for (auto &i : view)
        {
            REQUIRE(i == test[j]);
            j++;
        }
    }
}

TEST_CASE("split string_view", "string")
{
    SECTION("normal")
    {
        const_string_view view("abc,def,gh,i,");
        auto rets = view.split(',', &LibAllocatorV);
        REQUIRE(rets.size() == 5);
        REQUIRE(rets[0] == "abc");
        REQUIRE(rets[1] == "def");
        REQUIRE(rets[2] == "gh");
        REQUIRE(rets[3] == "i");
        REQUIRE(rets[4] == "");
    }

    SECTION("empty")
    {
        const_string_view view(",");
        auto rets = view.split(',', &LibAllocatorV);
        REQUIRE(rets.size() == 2);
    }

    SECTION("single")
    {
        const_string_view view(" ");
        auto rets = view.split(',', &LibAllocatorV);
        REQUIRE(rets.size() == 1);
        REQUIRE(rets[0] == " ");
    }
}

TEST_CASE("split n string_view", "string")
{
    SECTION("normal")
    {
        const_string_view view("abc,def,gh,i,");
        const_string_view views[4];
        auto cnt = view.split_n<4>(',', views);
        REQUIRE(cnt == 4);
        REQUIRE(views[3] == "i");
    }

    SECTION("empty list")
    {
        const_string_view view(",,,");
        const_string_view views[4];
        auto cnt = view.split_n<4>(',', views);
        REQUIRE(cnt == 4);
        REQUIRE(views[3] == "");
        REQUIRE(views[2] == "");
    }

    SECTION("empty string")
    {
        const_string_view view("");
        const_string_view views[4];
        auto cnt = view.split_n<4>(',', views);
        REQUIRE(cnt == 0);
    }
}

TEST_CASE("find string", "string")
{
    SECTION("find")
    {
        const_string_view view("hello \0\1world", 13);
        REQUIRE(view.find('o') == view.begin() + 4);
        REQUIRE(view.find('a') == view.end());
        REQUIRE(view.rfind('o') == view.begin() + 9);
    }

    SECTION("findstr")
    {
        const_string_view view("it's freelibcxx in freestanding, \0review it\5", 44);

        REQUIRE(view.find_substr("lo w") == view.end());

        REQUIRE(view.find_substr("i") == view.begin());
        REQUIRE(view.find_substr("'s") == view.begin() + 2);

        REQUIRE(view.find_substr("freelibcxx") == view.begin() + 5);

        REQUIRE(view.find_substr("freestanding,") == view.begin() + 19);
        REQUIRE(view.find_substr("freestandivg,") == view.end());
        REQUIRE(view.find_substr("it's freelibcxx in free") == view.begin());
        REQUIRE(view.find_substr("revi") == view.begin() + 34);
        REQUIRE(view.find_substr("review it") == view.begin() + 34);

        // rfind
        REQUIRE(view.rfind_substr("review it") == view.begin() + 34);
        REQUIRE(view.rfind_substr("revi") == view.begin() + 34);
        REQUIRE(view.rfind_substr("it's freelibcxx in free") == view.begin());
        REQUIRE(view.rfind_substr("freestandivg,") == view.end());
        REQUIRE(view.rfind_substr("'s") == view.begin() + 2);
        REQUIRE(view.rfind_substr("i") == view.end() - 3);
        REQUIRE(view.rfind_substr("it") == view.begin() + 41);
    }
}