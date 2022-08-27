#include "freelibcxx/unicode.hpp"
#include "common.hpp"
#include "freelibcxx/span.hpp"
#include "freelibcxx/string.hpp"
#include <catch2/catch_test_macros.hpp>
#include <codecvt>
#include <locale>

using namespace freelibcxx;

TEST_CASE("utf8 to unicode", "unicode")
{
    std::vector<const char *> vec{"a", "是", "안", "體", "ほ", "\n", "\1"};
    for (auto &chars : vec)
    {
        auto unicode = utf8_to_unicode(span<const char>(chars, strlen(chars)));
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
        auto ret = conv.from_bytes(chars);
        REQUIRE(unicode.has_value());
        REQUIRE(ret.size() == 1);
        unsigned int val = ret[0];
        REQUIRE(unicode.value() == val);
    }
}

TEST_CASE("unicode to utf8", "unicode")
{
    std::vector<char32_t> vec{0x1, 32, 128, 256, 257, 511, 512, 0x7FF, 0x800, 0xFFFF, 0x1'0000, 0x10'FFFF};
    char buf[5];

    for (auto codepoint : vec)
    {
        span<char> span(buf, 5);
        auto utf8 = unicode_to_utf8(codepoint, span);
        REQUIRE(utf8.has_value());
        auto utf8val = utf8.value();

        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
        auto ret = conv.to_bytes(codepoint);
        REQUIRE(ret.size() == utf8val.size());
        REQUIRE(strncmp(ret.data(), utf8val.get(), ret.size()) == 0);
    }
}