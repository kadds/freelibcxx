#include "freelibcxx/time.hpp"
#include "common.hpp"
#include "freelibcxx/string.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace freelibcxx;

std::vector<std::pair<uint64_t, const char *>> map{
    {1659334516UL, "2022-08-01 06:15:16"}, {1640995200UL, "2022-01-01 00:00:00"}, {1640930400UL, "2021-12-31 06:00:00"},
    {1640930400UL, "2021-12-31 06:00:00"}, {0UL, "1970-01-01 00:00:00"},          {86400UL, "1970-01-02 00:00:00"},
    {63072000UL, "1972-01-01 00:00:00"},   {63071999UL, "1971-12-31 23:59:59"},   {1582934400UL, "2020-02-29 00:00:00"},

};

TEST_CASE("format timestamp", "time")
{
    string ss(&LibAllocatorV);
    ss.resize(20);

    for (const auto [ts, str] : map)
    {
        tm_t t = tm_t::from_posix_seconds(ts).value();
        auto span = t.format(ss.span());
        REQUIRE(const_string_view(span) == str);
    }
}

TEST_CASE("from format time", "time")
{
    for (const auto [ts, str] : map)
    {
        const_string_view view(str);
        tm_t t = tm_t::from(view.span()).value_or(tm_t());
        REQUIRE(t.to_posix_seconds() == ts);
    }
}
