/**
 * @file utils.h
 * @brief Some subroutines useful for pseudorandom number generators
 * implementation.
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_UTILS_H
#define __SMOKERAND_UTILS_H

/**
 * @brief pcg_rxs_m_xs64 PRNG that has a good quality and can be used
 * for initialization for other PRNGs such as lagged Fibonacci.
 */
static inline uint64_t pcg_bits64(uint64_t *state)
{
    uint64_t word = ((*state >> ((*state >> 59) + 5)) ^ *state) *
        12605985483714917081ull;
    *state = *state * 6364136223846793005ull + 1442695040888963407ull;
    return (word >> 43) ^ word;
}

/**
 * @brief Expands a 64-bit seed to an array of 32-bit unsigned integers.
 * @details It uses the following two-step algorithm for expansion
 *
 * 1. Hash the seed using a custom full-period LFSR (xorrot64)
 * 2. Use the hashed seed for a PRNG based on a scrambled 64-bit Klimov-Shamir
 *    TF0 ("crazy" T-function). That PRNG is strong enough to pass BigCrush,
 *    PractRand and SmokeRand.
 *
 * @param x     Pointer to the output array.
 * @param len   Output array size.
 * @param seed  Seed to be expanded.
 */
static inline void expand_seed64_to_u32(uint32_t *x, size_t len, uint64_t seed)
{
    uint64_t u = seed;
    // Hash the seed using LFSR (xorrot64)
    for (int i = 0; i < 8; i++) {
        u ^= u << 5;
        u ^= rotl64(u, 13) ^ rotl64(u, 47);
    }
    // Expand the seed
    for (size_t i = 0; i < len; i++) {
        u += u * u | 0x40000005;
        const uint64_t y = 6906969069U * (u ^ (u >> 32));
        x[i] = (uint32_t) ((y ^ rotl64(y, 17) ^ rotl64(y, 53)) >> 32);
    }
}

/**
 * @brief Expands a 64-bit seed to an array of 64-bit unsigned integers.
 * @details It uses the following two-step algorithm for expansion
 *
 * 1. Hash the seed using a custom full-period LFSR (xorrot64)
 * 2. Use the hashed seed for a PRNG based on a scrambled 64-bit Klimov-Shamir
 *    TF0 ("crazy" T-function). That PRNG is strong enough to pass BigCrush,
 *    PractRand and SmokeRand.
 *
 * @param x     Pointer to the output array.
 * @param len   Output array size.
 * @param seed  Seed to be expanded.
 */
static inline void expand_seed64_to_u64(uint64_t *x, size_t len, uint64_t seed)
{
    uint64_t u = seed;
    // Hash the seed using LFSR (xorrot64)
    for (int i = 0; i < 8; i++) {
        u ^= u << 5;
        u ^= rotl64(u, 13) ^ rotl64(u, 47);
    }
    // Expand the seed
    for (size_t i = 0; i < len; i++) {
        u += u * u | 0x40000005;
        const uint64_t y = 6906969069U * (u ^ (u >> 32));
        x[i] = (y ^ rotl64(y, 17) ^ rotl64(y, 53)) >> 32;
    }
}

#endif // __SMOKERAND_UTILS_H
