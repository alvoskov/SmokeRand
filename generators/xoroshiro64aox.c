/**
 * @file xoroshiro64aox.c
 * @brief xoroshiro64aox is a LFSR generator with a simple output scrambler.
 * @details The algorithm is developed by David Blackman and Sebastiano Vigna
 * (vigna@acm.org) in 2016. An output AOX (addition-or-xor) scrambler
 * is developed by J. Hanlon and S. Felix to be friendly to hardware
 * implementation. It removes low linear complexity even from the lowest bit
 * but is weaker than ++ or ** scramblers in Hamming weights dependencies
 * removal.
 *
 * Only xoroshiro128aox with the 64-bit output was suggested in [1]. The
 * xoroshiro64aox modification with the 32-bit output was developed
 * by A. L. Voskov.
 *
 * References:
 *
 * 1. Hanlon J., Felix S. A Fast Hardware Pseudorandom Number Generator Based on
 *    xoroshiro128 // IEEE Transactions on Computers. 2023. V. 72. N 5.
 *    P.1518-1528. https://doi.org/10.1109/TC.2022.3204226
 * 2. https://prng.di.unimi.it/xoroshiro64star.c
 * 3. https://prng.di.unimi.it/xoroshiro64starstar.c
 *
 * @copyright xoroshiro64 algorithmû was suggested by D. Blackman
 * and S. Vigna, the AOX output scrambler was developed by J. Hanlon
 * and S. Felix. Adaptation of the scrambler to the 32-bit output and
 * reentrant version for SmokeRand:
 * 
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief xoroshiro64aox generator state.
 */
typedef struct {
    uint32_t s[2];    
} Xoroshiro64AoxState;


static inline uint64_t get_bits_raw(Xoroshiro64AoxState *obj)
{
    const uint32_t s0 = obj->s[0];
    uint32_t s1 = obj->s[1];
    const uint32_t sx = s0 ^ s1, sa = s0 & s1;
    const uint32_t result = sx ^ (rotl32(sa, 1) | rotl32(sa, 2));
    s1 ^= s0;
    obj->s[0] = rotl32(s0, 26) ^ s1 ^ (s1 << 9); // a, b
    obj->s[1] = rotl32(s1, 13); // c
    return result;
}

static void *create(const CallerAPI *intf)
{
    Xoroshiro64AoxState *obj = intf->malloc(sizeof(Xoroshiro64AoxState));
    uint64_t seed = intf->get_seed64();
    obj->s[0] = (uint32_t) (seed >> 32);
    obj->s[1] = (uint32_t) (seed & 0xFFFFFFFF);
    if (obj->s[0] == 0 && obj->s[1] == 0) {
        obj->s[0] = 0x12345678;
        obj->s[1] = 0xDEADBEEF;
    }
    return obj;
}

MAKE_UINT32_PRNG("xoroshiro64aox", NULL)
