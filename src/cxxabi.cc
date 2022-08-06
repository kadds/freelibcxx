#include <cstddef>

[[gnu::weak]] extern "C" void __cxa_pure_virtual()
{
    while (1)
        ;
}

// [[gnu::weak]] void operator delete(void *p, size_t log) {}

// [[gnu::weak]] void operator delete(void *p) {}
