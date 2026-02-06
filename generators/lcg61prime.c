/**
 * @file lcg61prime.c
 * @brief Just 61-bit LCG that returns the lower 32 bits.
 * @details
 *
 * Python script for test values generation:
 *
 *    x = 1
 *    for i in range(1000000):
 *        x = (1070922063159934167 * x) % (2**61 - 1)
 *    print(x)
 *
 * References:
 *
 * 1. P. L'Ecuyer. Tables of linear congruential generators of different
 *    sizes and good lattice structure // Mathematics of Computation. 1999.
 *    V. 68. N. 225. P. 249-260
 *    http://dx.doi.org/10.1090/S0025-5718-99-00996-5
 * 2. https://programmingpraxis.com/2014/01/14/minimum-standard-random-number-generator/
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(Lcg64State *obj)
{
    static const uint64_t m = 0x1FFFFFFFFFFFFFFFU; // 2^61 - 1
    uint64_t lo, hi;
    lo = unsigned_mul128(1070922063159934167U, obj->x, &hi);
    const uint64_t lo61 = lo & m, hi61 = (hi << 3) | (lo >> 61);
    obj->x = lo61 + hi61;
    if (obj->x >= m)
        obj->x -= m;
    return obj->x & 0xFFFFFFFFU;
}


static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64() | 0x1;
    return obj;
}


int run_self_test(const CallerAPI *intf)
{
    static const uint64_t x_ref = 561949181389516909U;
    Lcg64State obj;
    obj.x = 1;
    for (long i = 0; i < 1000000; i++) {
        get_bits_raw(&obj);
    }
    intf->printf("The current state is %llu, reference value is %llu\n",
        (unsigned long long) obj.x, (unsigned long long) x_ref);
    return (obj.x == x_ref) ? 1 : 0;
}


MAKE_UINT32_PRNG("Lcg61prime", run_self_test)
