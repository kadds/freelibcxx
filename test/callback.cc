#include "common.hpp"
#include "freelibcxx/delegate.hpp"
#include "freelibcxx/function_ref.hpp"
#include "freelibcxx/inplace_function.hpp"
#include <catch2/catch_test_macros.hpp>
#include <type_traits>

namespace
{
int add_one(int value) noexcept { return value + 1; }

int invoke_zero_argument(freelibcxx::function_ref<int()> function) { return function(); }

struct accumulator
{
    int value;

    int add(int amount) noexcept
    {
        value += amount;
        return value;
    }
};

struct counter
{
    int value = 0;

    void operator()() noexcept { value++; }
};
} // namespace

TEST_CASE("function_ref borrows functions and lvalue callables", "function_ref")
{
    static_assert(sizeof(freelibcxx::function_ref<int(int)>) == 2 * sizeof(void *));
    static_assert(std::is_nothrow_copy_constructible_v<freelibcxx::function_ref<int(int)>>);

    auto add = [](int value) { return value + 2; };
    freelibcxx::function_ref<int(int)> ref(add);
    REQUIRE(ref(3) == 5);

    freelibcxx::function_ref<int(int)> function_ref_to_function(add_one);
    REQUIRE(function_ref_to_function(3) == 4);
}

TEST_CASE("function_ref can be empty and copied", "function_ref")
{
    freelibcxx::function_ref<void()> empty;
    REQUIRE_FALSE(empty);

    auto increment = []() {};
    freelibcxx::function_ref<void()> first(increment);
    auto second = first;
    REQUIRE(first);
    REQUIRE(second);
    REQUIRE(invoke_zero_argument([] { return 7; }) == 7);
}

TEST_CASE("delegate binds free and member functions", "delegate")
{
    using delegate = freelibcxx::delegate<int(int) noexcept>;
    static_assert(sizeof(delegate) == 2 * sizeof(void *));
    static_assert(std::is_nothrow_copy_constructible_v<delegate>);

    auto free_function = delegate::bind<&add_one>();
    REQUIRE(free_function(4) == 5);

    accumulator object{10};
    auto member_function = delegate::bind<&accumulator::add>(object);
    REQUIRE(member_function(5) == 15);
    REQUIRE(object.value == 15);
}

TEST_CASE("delegate borrows an lvalue callable", "delegate")
{
    counter callable;
    auto callback = freelibcxx::delegate<void() noexcept>::borrow(callable);
    callback();
    callback();
    REQUIRE(callable.value == 2);
}

TEST_CASE("inplace_function owns a bounded callable", "inplace_function")
{
    using work = freelibcxx::inplace_function<void() noexcept, 16>;
    static_assert(std::is_nothrow_move_constructible_v<work>);

    int value = 0;
    work first([&value]() noexcept { value += 3; });
    work second(std::move(first));
    second();
    REQUIRE(value == 3);
    REQUIRE(first == nullptr);
    REQUIRE(second);
}
