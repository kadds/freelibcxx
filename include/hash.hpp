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
    size_t operator()() const { return 0; }
};

} // namespace freelibcxx
