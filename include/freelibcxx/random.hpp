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
    constexpr void init(uint64_t seed);

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

inline constexpr void mt19937_random_engine::init(uint64_t seed)
{
    state_[0] = detail::xor_64bits(seed);
    for (int i = 1; i < N; i++)
    {
        state_[i] = 1812433253 * (state_[i - 1] ^ (state_[i - 1] >> 30)) + i;
    }
}

// https://en.wikipedia.org/wiki/Mersenne_Twister
inline uint64_t mt19937_random_engine::operator()()
{
    if (index_ >= N)
    {
        for (int i = 0; i < N - 1; i++)
        {
            uint32_t x = (state_[i] & 0x8000'0000) | (state_[(i + 1) % N] & 0x7FFF'FFFF);
            uint32_t xa = x >> 1;
            if ((x % 2) != 0)
            {
                xa ^= A;
            }
            state_[i] = state_[(i + M) % N] ^ xa;
        }
        index_ = 0;
    }
    uint32_t y = state_[index_];
    index_++;
    return y ^ y >> 11;
}

inline void linear_random_engine::update_seed(uint64_t seed) { seed_ = detail::xor_64bits(seed); }

inline uint64_t linear_random_engine::operator()()
{
    const uint32_t A = 1103515245;
    const uint32_t C = 12345;
    const uint32_t M = 1UL << 30;

    uint32_t ret0 = (seed_ * A + C) % M;
    uint32_t ret1 = (ret0 * A + C) % M;
    seed_ = ret1;
    return (ret0 | ((uint64_t)ret1 << 30)) & 0xFFFF'FFFF;
}

} // namespace freelibcxx
