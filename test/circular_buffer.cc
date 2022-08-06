#include "freelibcxx/circular_buffer.hpp"
#include "common.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace freelibcxx;

TEST_CASE("create circular buffer", "circular_buffer")
{
    circular_buffer<int> buf(&LibAllocatorV, 1024);
    REQUIRE(buf.capacity() == 1024);
    REQUIRE(buf.empty());
    REQUIRE(!buf.full());
}

TEST_CASE("read&write circular buffer", "circular_buffer")
{
    char data[10];
    circular_buffer<char> buf(&LibAllocatorV, 4);
    buf.write("123", 3);
    REQUIRE(buf.full());

    data[buf.read(data, sizeof(data))] = 0;
    REQUIRE(strcmp(data, "123") == 0);

    buf.write("45", 2);
    REQUIRE(buf.capacity_readable() == 2);
    REQUIRE(buf.capacity_writeable() == 1);

    data[buf.read(data, sizeof(data))] = 0;
    REQUIRE(strcmp(data, "45") == 0);

    buf.write("67890", 5);
    data[buf.read(data, sizeof(data))] = 0;
    REQUIRE(strcmp(data, "678") == 0);

    data[buf.read(data, sizeof(data))] = 0;
    REQUIRE(strcmp(data, "") == 0);
}
