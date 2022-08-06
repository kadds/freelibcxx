#pragma once
#include <cstddef>
#include <cstdint>
namespace freelibcxx
{

uint32_t djb_hash(const char *str);
size_t murmur_hash2_64(const void *key, size_t len, uint64_t seed);
void murmur_hash3_128(const void *key, const size_t len, const uint32_t seed, void *out);

template <typename T> class hasher
{
  public:
};

template <> struct hasher<unsigned long>
{
    size_t operator()(const unsigned long &t) { return murmur_hash2_64(&t, sizeof(unsigned long), 0); }
};

template <> struct hasher<unsigned int>
{
    size_t operator()(const unsigned int &t)
    {
        size_t k = t;
        return murmur_hash2_64(&k, sizeof(k), 0);
    }
};

template <> struct hasher<long>
{
    size_t operator()(const long &t) { return murmur_hash2_64(&t, sizeof(unsigned long), 0); }
};

template <> struct hasher<int>
{
    size_t operator()(const int &t)
    {
        size_t k = t;
        return murmur_hash2_64(&k, sizeof(k), 0);
    }
};

} // namespace freelibcxx
