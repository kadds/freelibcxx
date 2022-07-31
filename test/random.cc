#include "random.hpp"
#include "catch2/internal/catch_run_context.hpp"
#include "common.hpp"
#include <catch2/catch_test_macros.hpp>
using namespace freelibcxx;

template <typename E> void testing()
{
    E engine(Catch::rngSeed());

    random_generator rng(engine);
    rng();

    SECTION("range int")
    {
        for (int i = 0; i < 10000; i++)
        {
            auto val = rng.gen_range(-1, 10);
            REQUIRE(val >= -1);
            REQUIRE(val < 10);
        }
    }
    SECTION("range uint")
    {
        for (int i = 0; i < 10000; i++)
        {
            auto val = rng.gen_range(1UL, 10000UL);
            REQUIRE(val >= 1UL);
            REQUIRE(val < 10000UL);
        }
    }
    SECTION("range single") { REQUIRE(rng.gen_range(1UL, 2UL) == 1UL); }
}

TEST_CASE("mt19937 testing", "random") { testing<mt19937_random_engine>(); }
TEST_CASE("linear testing", "random") { testing<linear_random_engine>(); }
