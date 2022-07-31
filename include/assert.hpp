#pragma once
#include <source_location>
#include <utility>

namespace freelibcxx
{
[[gnu::weak]] void assert_fail(const char *expr, const char *file, int line, const char *msg);
}

#define CXXASSERT_MSG(expr, msg)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr)) [[unlikely]]                                                                                      \
        {                                                                                                              \
            if (::freelibcxx::assert_fail != nullptr) [[likely]]                                                       \
            {                                                                                                          \
                ::freelibcxx::assert_fail(#expr, __FILE__, __LINE__, msg);                                             \
            }                                                                                                          \
            else                                                                                                       \
            {                                                                                                          \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

#define CXXASSERT(expr) CXXASSERT_MSG(expr, "")
