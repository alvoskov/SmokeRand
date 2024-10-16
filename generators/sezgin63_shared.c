/**
 * @file sezgin63_shared.c
 * @brief Implementation of 63-bit LCG with prime modulus.
 * @details Gives suspicious values at some bspace tests.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief 63-bit LCG state.
 */
typedef struct {
    int64_t x; ///< Must be signed!
} Lcg63State;


static uint64_t get_bits(void *state)
{
    static const int64_t m = 9223372036854775783LL; // 2^63 - 25
    static const int64_t a = 3163036175LL; // See Line 4 in Table 1
    static const int64_t b = 2915986895LL;
    static const int64_t c = 2143849158LL;
    Lcg63State *obj = state;    
    obj->x = a * (obj->x % b) - c*(obj->x / b);
    if (obj->x < 0LL) {
        obj->x += m;
    }
    return obj->x >> 31;
}

static void *create(const CallerAPI *intf)
{
    Lcg63State *obj = intf->malloc(sizeof(Lcg63State));
    obj->x = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT32_PRNG("Sezgin63", NULL)

