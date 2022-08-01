#pragma once
#include <cstddef>

extern "C" void *memcpy(void *__restrict dest, const void *__restrict src, size_t n) noexcept;

extern "C" void *memmove(void *dest, const void *src, size_t n) noexcept;

extern "C" void *memset(void *dest, int val, size_t n) noexcept;

extern "C" size_t strlen(const char *s) noexcept;

extern "C" int memcmp(const void *a, const void *b, size_t size) noexcept;