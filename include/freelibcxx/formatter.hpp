#pragma once
#include "freelibcxx/optional.hpp"
#include "freelibcxx/span.hpp"
#include <cstddef>
#include <cstdint>

namespace freelibcxx
{
optional<int> str2int(span<const char> in, int base = 10);
optional<unsigned int> str2uint(span<const char> in, int base = 10);

optional<int64_t> str2int64(span<const char> in, int base = 10);
optional<uint64_t> str2uint64(span<const char> in, int base = 10);

optional<int> int2str(span<char> buffer, int val, int base = 10);
optional<int> uint2str(span<char> buffer, unsigned int val, int base = 10);

optional<int> int642str(span<char> buffer, int64_t val, int base = 10);
optional<int> uint642str(span<char> buffer, uint64_t val, int base = 10);

optional<int> double2str(span<char> buffer, double val, int precision);
optional<double> str2double(const char *ptr, char **end);
} // namespace freelibcxx