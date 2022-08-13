#pragma once
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
namespace freelibcxx
{
template <typename... Args> using tuple = std::tuple<Args...>;
template <typename... Args> tuple<typename std::remove_cvref_t<Args>...> make_tuple(Args &&...args)
{
    return std::make_tuple(std::forward<Args>(args)...);
}
} // namespace freelibcxx
