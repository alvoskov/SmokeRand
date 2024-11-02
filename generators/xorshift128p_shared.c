/**
 * @file xorshift128p_shared.c
 * @brief An implementation of 128-bit LSFR generator proposed by G. Marsaglia.
 * @details 
 *
 * References:
 * 
 * 1. Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *    V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 * 2. Vigna S. Further scramblings of Marsaglia's xorshift generators //
 *    Journal of Computational and Applied Mathematics. 2017. V. 315.
 *    P.175-181. https://doi.org/10.1016/j.cam.2016.11.006.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Xorshift128+ PRNG state
 */
typedef struct {
    uint64_t s[2];
} Xorshift128PlusState;


static inline uint64_t get_bits_raw(void *state)
{
    Xorshift128PlusState *obj = state;
    uint64_t s1 = obj->s[0];
    const uint64_t s0 = obj->s[1];
    const uint64_t result = s0 + s1;
    obj->s[0] = s0;
    s1 ^= s1 << 23; // a
    obj->s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5); // b, c
    return result;
}


static void *create(const CallerAPI *intf)
{
    Xorshift128PlusState *obj = intf->malloc(sizeof(Xorshift128PlusState));
    obj->s[0] = intf->get_seed64();
    obj->s[1] = intf->get_seed64();
    if (obj->s[0] == 0 && obj->s[1] == 0) {
        obj->s[1] = 0x9E3779B97F4A7C15;
    }
    return (void *) obj;
}

MAKE_UINT64_PRNG("Xorshift128+", NULL)
