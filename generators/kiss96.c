/**
 * @file kiss96.c
 * @brief KISS generator: the version from DIEHARD test suite.
 * @details This version of KISS generator was suggested by G.Marsaglia
 * and included in his DIEHARD test suite. It is a combined generator
 * made from 32-bit LCG ("69069"), xorshift32 and some generalized
 * multiply-with-carry PRNG:
 *
 * \f[
 * z_{n} = 2z_{n-1} + z_{n-2} + c_{n-1} \mod 2^{32}
 * \f]
 *
 * The algorithm for this MWC is taken from DIEHARD FORTRAN source code
 * and uses some bithacks to implement it without 64-bit data types.
 *
 * The original FORTRAN source code:
 *
 *    do 33 jj=1,4096
 *    x=69069*x+1
 *    y=xor(y,lshift(y,13))
 *    y=xor(y,rshift(y,17))
 *    y=xor(y,lshift(y,5))
 *    k=rshift(z,2)+rshift(w,3)+rshift(carry,2)
 *    m=w+w+z+carry
 *    z=w
 *    w=m
 *    carry=rshift(k,30)
 *    33              b(jj)=x+y+w
 *
 * It seems that the original code (and its version for C) contains an
 * implementation error.
 *
 * @copyright The KISS96 algorithm was developed by George Marsaglia.
 *
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t x; ///< LCG state.
    uint32_t y; ///< SHR3 state.
    uint32_t z; ///< MWC state: value.
    uint32_t w; ///< MWC state: value.
    uint32_t c; ///< MWC state: carry.
} Kiss96State;



static void Kiss96State_init(Kiss96State *obj, uint64_t seed)
{
    const uint32_t seed_lo = (uint32_t) seed;
    const uint32_t seed_hi = (uint32_t) (seed >> 32);
    obj->x = seed_lo;
    obj->y = seed_hi;
    if (obj->y == 0){
        obj->y = 0x12345678;
    }
    obj->z = (uint32_t) (seed_hi & 0xFFFF);
    obj->w = (uint32_t) (seed_hi >> 16);
    obj->c = 0;
}


/*
*/

static inline uint64_t get_bits_raw(Kiss96State *obj)
{
    obj->x = obj->x * 69069u + 1u;
    obj->y ^= obj->y << 13;
    obj->y ^= obj->y >> 17;
    obj->y ^= obj->y << 5;
#if SIZE_MAX <= UINT32_MAX
    const uint32_t lo = obj->w + obj->w + obj->z + obj->c;
    const uint32_t csum = ((obj->w << 1) & 0x3)
        + (obj->z & 0x3) + obj->c;
    const uint32_t hi = (obj->z >> 2) + (obj->w >> 1) + (csum >> 2);
    obj->z = obj->w;
    obj->w = lo;
    obj->c = hi >> 30;
#else
    const uint64_t m = 2U*(uint64_t)obj->w + (uint64_t)obj->z + (uint64_t)obj->c;
    obj->z = obj->w;
    obj->w = (uint32_t)m;
    obj->c = (uint32_t)(m >> 32);
#endif
    return obj->x + obj->y + obj->w;
}

static void *create(const CallerAPI *intf)
{
    Kiss96State *obj = intf->malloc(sizeof(Kiss96State));
    Kiss96State_init(obj, intf->get_seed64());
    return obj;
}


/**
 * @brief An internal self-test
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint32_t refval = 3320424011U;
    uint32_t val = 0;
    Kiss96State obj = {.x = 12345, .y = 54321,
        .z = 67890, .w = 0xABCDEF, .c = 1};
    for (long i = 1; i < 1000000; i++) {
        val = (uint32_t) get_bits_raw(&obj);
        //intf->printf("%X %X %X\n", obj.z, obj.w, obj.c);
    }
    intf->printf("Reference value: %u\n", refval);
    intf->printf("Obtained value:  %u\n", val);
    intf->printf("Difference:      %u\n", refval - val);
    return refval == val;
}


MAKE_UINT32_PRNG("KISS96", run_self_test)
