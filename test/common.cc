#include "common.hpp"
#include "freelibcxx/assert.hpp"
#include <catch2/catch_test_macros.hpp>
#include <exception>
#include <sstream>
#include <stdexcept>
LibAllocator LibAllocatorV;

namespace freelibcxx
{
void assert_fail(const char *expr, const char *file, int line, const char *msg)
{
    std::stringstream ss;
    ss << "assert \"" << expr << "\" fail\n  " << msg << " at " << file << ":" << line;

    throw std::runtime_error(ss.str());
}
} // namespace freelibcxx