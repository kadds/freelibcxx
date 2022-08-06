#include "freelibcxx/tuple.hpp"
#include "common.hpp"
#include <catch2/catch_test_macros.hpp>
using namespace freelibcxx;

TEST_CASE("tuple create", "tuple")
{
    tuple<int, int> tuple(1, 2);
    REQUIRE(std::get<0>(tuple) == 1);
    REQUIRE(std::get<1>(tuple) == 2);
}