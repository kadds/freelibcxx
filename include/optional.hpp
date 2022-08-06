#pragma once
#include <optional>
namespace freelibcxx
{
template <typename T> using optional = std::optional<T>;
using nullopt_t = std::nullopt_t;
inline constexpr auto nullopt = std::nullopt;

} // namespace freelibcxx