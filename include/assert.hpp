#pragma once
#include <source_location>
#include <utility>

#ifdef cassert
#undef cassert
#endif

namespace freelibcxx
{
void cassert_fail(const char *expr, const char *file, int line);
}
#define cassert(expr)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr)) [[unlikely]]                                                                                      \
        {                                                                                                              \
            ::freelibcxx::cassert_fail(#expr, __FILE__, __LINE__);                                                     \
        }                                                                                                              \
    } while (0)
