#pragma once
#include <cstddef>
#include <cstdint>

namespace freelibcxx
{
namespace detail
{
constexpr uint32_t xor_64bits(uint64_t val) { return ((uint32_t)val) ^ ((uint32_t)(val << 32)); }
} // namespace detail

template <typename E> class random_generator
{
  public:
    random_generator(E &engine)
        : engine_(engine)
    {
    }

    // [min, max)
    template <typename T> inline T gen_range(T min, T max)
    {
        if (max <= min)
        {
            // TODO: throw Exception ?
        }
        return operator()() % (max - min) + min;
    }

    size_t operator()() { return engine_(); }

  private:
    E &engine_;
};

// output 32bits
class mt19937_random_engine
{
    static constexpr uint32_t A = 0x9908B0DF;
    static constexpr uint32_t M = 397;
    static constexpr uint32_t W = 32;
    static constexpr int N = 624;
    static constexpr uint32_t R = 31;

  public:
    mt19937_random_engine(uint64_t seed)
        : index_(0)
    {
        init(seed);
    }

    uint64_t operator()();

    uint64_t pick() const { return state_[index_]; };

  private:
    constexpr void init(uint64_t seed)
    {
        state_[0] = detail::xor_64bits(seed);
        for (int i = 1; i < N; i++)
        {
            state_[i] = 1812433253 * (state_[i - 1] ^ (state_[i - 1] >> 30)) + i;
        }
    }

    uint32_t index_;
    uint32_t state_[N];
    static mt19937_random_engine global_;
};

// output 32bits
class linear_random_engine
{
  public:
    linear_random_engine(uint64_t seed) { update_seed(seed); }

    void update_seed(uint64_t seed);
    uint64_t operator()();

    uint64_t pick() const { return seed_; };

  private:
    uint64_t seed_;
    static linear_random_engine global_;
};

} // namespace freelibcxx
