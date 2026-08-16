#include "freelibcxx/formatter.hpp"
#include "common.hpp"
#include "freelibcxx/string.hpp"
#include <catch2/catch_test_macros.hpp>
using namespace freelibcxx;

static_assert(format_detail::valid_literal<int>("{:08x}", 6));
static_assert(!format_detail::valid_literal<int>("{:65x}", 6));
static_assert(!format_detail::valid_literal<const char *>("{:d}", 4));

TEST_CASE("format int", "formatter")
{
    string ss(&LibAllocatorV);
    // uint
    ss << 123U;
    REQUIRE(ss == "123");
    ss.clear();

    // int
    ss << -1958;
    REQUIRE(ss == "-1958");
    ss.clear();

    // int64
    ss << -12345678901234567;
    REQUIRE(ss == "-12345678901234567");
    ss.clear();

    // uint64
    ss << 12345678901234567UL;
    REQUIRE(ss == "12345678901234567");
    ss.clear();

    // int overflow
    ss << 2'147'483'647;
    REQUIRE(ss == "2147483647");
    ss.clear();
    ss << -2'147'483'648;
    REQUIRE(ss == "-2147483648");
    ss.clear();
}

TEST_CASE("from int", "formatter")
{
    const_string_view ss;
    ss = {"123"};
    REQUIRE(ss.to_int().value_or(0) == 123);

    ss = {"-123"};
    REQUIRE(ss.to_int().value_or(0) == -123);

    ss = {"-123"};
    REQUIRE(ss.to_uint().value_or(0) == 0);

    ss = {"-123456789012345678"};
    REQUIRE(ss.to_int64().value_or(0) == -123456789012345678);

    ss = {"1 02"};
    REQUIRE(ss.to_int64().value_or(0) == 0);

    ss = {"--4"};
    REQUIRE(ss.to_int64().value_or(0) == 0);

    ss = {"+1"};
    REQUIRE(ss.to_int64().value_or(0) == 1);

    ss = {"0"};
    REQUIRE(ss.to_int64().value_or(1) == 0);

    ss = {"-0"};
    REQUIRE(ss.to_int64().value_or(1) == 0);

    ss = {"a"};
    REQUIRE(ss.to_int64().value_or(0) == 0);

    ss = {"123456788901"};
    REQUIRE(ss.to_int().value_or(0) == 0);

    ss = {"-2147483648"};
    REQUIRE(ss.to_int().value_or(0) == -2147483648);

    ss = {"2147483648"};
    REQUIRE(ss.to_int().value_or(0) == 0);

    ss = {"2147483647"};
    REQUIRE(ss.to_int().value_or(0) == 2147483647);
}

TEST_CASE("format hex", "formatter")
{
    string ss(&LibAllocatorV);
    ss.from_int64(9, 16);
    REQUIRE(ss == "9");
    ss.clear();

    ss.from_int64(10, 16);
    REQUIRE(ss == "a");
    ss.clear();

    ss.from_int64(254, 16);
    REQUIRE(ss == "fe");
    ss.clear();

    ss.from_uint(257, 16);
    REQUIRE(ss == "101");
    ss.clear();
}

TEST_CASE("from hex", "formatter")
{
    const_string_view ss;
    ss = {"f"};
    REQUIRE(ss.to_int(16).value_or(0) == 15);

    ss = {"fe"};
    REQUIRE(ss.to_int(16).value_or(0) == 254);

    ss = {"FE"};
    REQUIRE(ss.to_int(16).value_or(0) == 254);

    ss = {"101"};
    REQUIRE(ss.to_int(16).value_or(0) == 257);

    // invalid input
    ss = {"fg"};
    REQUIRE(ss.to_int(16).value_or(0) == 0);
}

TEST_CASE("bounded brace formatter", "formatter")
{
    char buffer[64]{};
    auto result = format_to(span<char>(buffer, sizeof(buffer) - 1), "{} {:08x}", -7, 0xabU);

    REQUIRE(result.error == format_error::none);
    REQUIRE_FALSE(result.truncated);
    buffer[result.written] = '\0';
    REQUIRE(std::string_view(buffer) == "-7 000000ab");
}

TEST_CASE("bounded brace formatter escapes and truncates", "formatter")
{
    char buffer[8]{};
    auto result = format_to(span<char>(buffer, sizeof(buffer) - 2), "{{{}}}", "hello");

    REQUIRE(result.error == format_error::none);
    REQUIRE(result.truncated);
    buffer[result.written] = '\0';
    REQUIRE(std::string_view(buffer) == "{hello");
}

TEST_CASE("bounded brace formatter stops char arrays at their terminator", "formatter")
{
    char value[32] = "hello";
    char buffer[64]{};
    auto result = format_to(span<char>(buffer, sizeof(buffer) - 1), "{}", value);

    REQUIRE(result.error == format_error::none);
    REQUIRE_FALSE(result.truncated);
    buffer[result.written] = '\0';
    REQUIRE(std::string_view(buffer) == "hello");
}

TEST_CASE("bounded brace formatter formats decimal hex and pointers", "formatter")
{
    char buffer[96]{};
    auto result = format_to(span<char>(buffer, sizeof(buffer) - 1), "{:d} {:x} {:X} {:p}", -42, 0xabU, 0xabU,
                            static_cast<void *>(buffer));

    REQUIRE(result.error == format_error::none);
    REQUIRE_FALSE(result.truncated);
    buffer[result.written] = '\0';
    const std::string_view rendered(buffer);
    REQUIRE(rendered.starts_with("-42 ab AB 0x"));
}
