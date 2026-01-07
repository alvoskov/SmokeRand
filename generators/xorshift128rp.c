/**
 * @file xorshift128rp.c
 * @brief A chaotic generator inspired by xorshift family PRNG.
 * @details It resembles xorshift but includes an addition that eliminates
 * issues with low linear complexity. However, it has no theoretical proofs
 * of minimal period and it fails the `hamming_distr` test in the `default`
 * battery and several other tests in the `full` battery.
 *
 * References:
 * 
 * 1. ÇABUK, UMUT CAN; AYDIN, ÖMER; and DALKILIÇ, GÖKHAN "A random number
 *    generator for lightweight authentication protocols: xorshiftR+ // Turkish
 *    Journal of Electrical Engineering and Computer Sciences. 2017. Vol. 25.
 *  No. 6. Article 31. https://doi.org/10.3906/elk-1703-361 
 * 2. Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *    V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 *
 * @copyright The xorshift128r+ algorithm is suggested by Çabuk, Aydin et al.
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Xorshift128R+ PRNG state
 */
typedef struct {
    uint64_t s[2];
} Xorshift128RpState;


static inline uint64_t get_bits_raw(Xorshift128RpState *obj)
{
    uint64_t x = obj->s[0];
    uint64_t const y = obj->s[1];
    obj->s[0] = y;
    x ^= x << 23;
    x ^= x >> 17;
    x ^= y;
    obj->s[1] = x + y;
    return x;
}


static void *create(const CallerAPI *intf)
{
    Xorshift128RpState *obj = intf->malloc(sizeof(Xorshift128RpState));
    obj->s[0] = intf->get_seed64();
    obj->s[1] = intf->get_seed64();
    if (obj->s[0] == 0 && obj->s[1] == 0) {
        obj->s[1] = 0x9E3779B97F4A7C15;
    }
    return obj;
}

MAKE_UINT64_PRNG("Xorshift128R+", NULL)
