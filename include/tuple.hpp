#pragma once
#include <bits/utility.h>
#include <cstddef>
#include <tuple>
#include <utility>
namespace freelibcxx
{
namespace detail
{

template <typename... Args> using tuple = std::tuple<Args...>;
} // namespace freelibcxx