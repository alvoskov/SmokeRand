/**
 * @file kiss93.c
 * @brief KISS93 pseudorandom number generator. It passes SmallCrush
 * but fails the LinearComp (r = 29) test in the Crush battery (N72).
 *
 * References:
 *
 * 1. G. Marsaglia, A. Zaman. Monkey tests for random number generators //
 *    // Computers & Mathematics with Applications. 1993. V. 26. N 9.
 *    P. 1-10. https://doi.org/10.1016/0898-1221(93)90001-C
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 *
 * The KISS93 algorithm is developed by George Marsaglia.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief KISS93 PRNG state.
 */
typedef struct {
    uint32_t lcg;
    uint32_t xs1;
    uint32_t xs2;
} KISS93State;

static inline uint64_t get_bits_raw(KISS93State *obj)
{
    // LCG
    obj->lcg = 69069U * obj->lcg + 23606797U;
    // Some LFSRs
    // a) LFSR 1: b = b*(I + L**17)*(I + R**15) for 32-bit words
    obj->xs1 ^= obj->xs1 << 17;
    obj->xs1 ^= obj->xs2 >> 15;
    // b) LFSR 2: b = b*(I + L**18)*(I + R**13) for 31-bit words
    obj->xs2 = ((obj->xs2 << 18) ^ obj->xs2) & 0x7fffffffU;
    obj->xs2 ^= obj->xs2 >> 13;
    // Output function
    return obj->lcg + obj->xs1 + obj->xs2;
}


static void *create(const CallerAPI *intf)
{
    // Default seeds: 12345, 6789, 111213
    KISS93State *obj = intf->malloc(sizeof(KISS93State));
    seed64_to_2x32(intf, &obj->lcg, &obj->xs2);
    obj->xs1 = 6789; // The polynomial is not primitive, the default
                     // value will give the period close to 2^32.
    if (obj->xs2 == 0) { // The polynomial is primitive
        obj->xs2 = 111213;
    }
    return obj;
}

MAKE_UINT32_PRNG("KISS93", NULL)
