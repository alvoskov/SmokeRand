/**
 * @file minstd.c
 * @brief Obsolete "minimal standard" 31-bit LCG with prime modulus.
 * It is \f$ LCG(2^{31} - 1, 16807, 0) \f$. Fails SmallCrush, Crush,
 * BigCrush and PractRand and fairly slow on modern 64-bit processors. 
 * @details This implementation of minstd is based on "Integer version 2"
 * from [1]. It uses Schrage's method to be able to use only 32-bit
 * arithmetics.
 *
 * Reference:
 *
 * 1. S. K. Park, K. W. Miller. Random number generators: good ones
 *    are hard to find // Communications of the ACM. 1988. V. 31. N 10.
 *    P.1192-1201. https://doi.org/10.1145/63039.63042
 * 2. https://programmingpraxis.com/2014/01/14/minimum-standard-random-number-generator/
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(void *state)
{
/*
    static const int32_t m = 2147483647, a = 16807, q = 127773, r = 2836;
    Lcg32State *obj = state;
    const int32_t x = (int32_t) obj->x;
    const int32_t hi = x / q;
    const int32_t lo = x - q * hi; // It is x mod q
    const int32_t t = a * lo - r * hi;
    obj->x = (uint32_t) ((t < 0) ? (t + m) : t);
*/

/*
    Lcg32State *obj = state;
    obj->x = (uint32_t) ( (16807ULL * obj->x) % 2147483647ULL );
*/

    Lcg32State *obj = state;
    uint64_t prod = 16807ULL * obj->x;
    uint32_t q = (uint32_t) (prod & 0x7FFFFFFFU);
    uint32_t p = (uint32_t) (prod >> 31);
    obj->x = p + q;
    if (obj->x >= 0x7FFFFFFFU) {
        obj->x -= 0x7FFFFFFFU;
    }

    return obj->x << 1;
}


static void *create(const CallerAPI *intf)
{
    Lcg32State *obj = intf->malloc(sizeof(Lcg32State));
    obj->x = (uint32_t) (intf->get_seed64() >> 33);
    return (void *) obj;
}


int run_self_test(const CallerAPI *intf)
{
    const uint32_t x_ref = 1043618065;
    Lcg32State obj;
    obj.x = 1;
    for (size_t i = 0; i < 10000; i++) {
        get_bits_raw(&obj.x);
    }
    intf->printf("The current state is %d, reference value is %d\n",
        (int) obj.x, (int) x_ref);
    return obj.x == x_ref;
}


MAKE_UINT32_PRNG("Minstd", run_self_test)
