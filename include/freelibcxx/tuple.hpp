#pragma once
#include <cstddef>
#include <tuple>
#include <utility>
namespace freelibcxx
{
namespace detail
{

template <typename... Args> using tuple = std::tuple<Args...>;
} // namespace freelibcxx
} // namespace freelibcxx
