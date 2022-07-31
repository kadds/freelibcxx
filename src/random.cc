#include "random.hpp"

namespace freelibcxx
{
// class liner_random_generator_impl
// {
//     // void operator() {}
// };

// random_generator random_generator::global_;

// size_t random_generator::get() { return seed_++; }
// random_generator &random_generator::get_global() { return global_; }

// https://en.wikipedia.org/wiki/Mersenne_Twister
uint64_t mt19937_random_engine::operator()()
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

void linear_random_engine::update_seed(uint64_t seed) { seed_ = detail::xor_64bits(seed); }

uint64_t linear_random_engine::operator()()
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
