/**
 * @file jkiss32.c
 * @brief JKISS32 is a version of KISS algorithm (2007 version) by J. Marsaglia
 * with parameteres tuned by David Jones.
 * @details It doesn't use multiplication: it is a combination or xorshift32,
 * discrete Weyl sequence and AWC (add with carry) generator. Doesn't require
 * 64-bit integers.
 *
 * The next comment by G. Marsaglia should be taken into account during its
 * initialization:
 *
 * When b=2^31, b^2+b-1 is not prime, but factors into 610092078393289 * 7559
 * so the full period from KISS() requires that for the seeds x, y, z, w, c:
 *
 * - x can be any 32-bit integer,
 * - y can be any 32-bit integer not 0,
 * - z and w any 31-bit integers not multiples of 7559
 * - c can be 0 or 1.
 *
 * References:
 *
 * 1. David Jones, UCL Bioinformatics Group. Good Practice in (Pseudo) Random
 *    Number Generation for Bioinformatics Applications
 *    http://www0.cs.ucl.ac.uk/staff/D.Jones/GoodPracticeRNG.pdf
 * 2. https://groups.google.com/g/comp.lang.fortran/c/5Bi8cFoYwPE
 * 3. https://talkchess.com/viewtopic.php?t=38313&start=10
 *
 * @copyright
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 *
 * The KISS algorithm is developed by George Marsaglia, its JKISS version
 * was suggested by David Jones.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief JKISS32 PRNG state.
 */
typedef struct {
    uint32_t x; ///< Discrete Weyl sequence part.
    uint32_t y; ///< xorshift part.
    uint32_t z; ///< AWC part 1
    uint32_t w; ///< AWC part 2
    uint32_t c; ///< AWC carry bit
} JKISS32State;


static inline uint64_t get_bits_raw(JKISS32State *obj)
{
    // xorshift part
    obj->y ^= obj->y << 5;
    obj->y ^= obj->y >> 7;
    obj->y ^= obj->y << 22;
    // AWC (add with carry) part
    // Note: sign bit hack!
    const int32_t t = (int32_t) (obj->z + obj->w + obj->c);
    obj->z = obj->w;
    obj->c = t < 0;
    obj->w = t & 0x7fffffff;
    // Discrete Weyl sequence part
    obj->x += 1411392427;
    // Combined output
    return obj->x + obj->y + obj->w;
}


static void *create(const CallerAPI *intf)
{
    static const uint32_t AWC_MAX = 0x7fffffff; // 2^31 - 1
    JKISS32State *obj = intf->malloc(sizeof(JKISS32State));
    seed64_to_2x32(intf, &obj->x, &obj->y);
    if (obj->y == 0) {
        obj->y = 0x12345678;
    }
    seed64_to_2x32(intf, &obj->z, &obj->w);
    obj->z = obj->z & AWC_MAX;
    obj->w = obj->w & AWC_MAX;
    while (obj->z % 7559 == 0) { obj->z--; obj->z = (obj->z & AWC_MAX); }
    while (obj->w % 7559 == 0) { obj->w--; obj->w = (obj->w & AWC_MAX); }
    obj->c = 0;
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t x_ref = 2362004368;
    uint32_t x;
    JKISS32State *obj = intf->malloc(sizeof(JKISS32State));
    obj->x = 123456789;
    obj->y = 234567891;
    obj->z = 345678912;
    obj->w = 456789123;
    obj->c = 0;
    for (long i = 0; i < 10000000; i++) {
        x = (uint32_t) get_bits_raw(obj);
    }
    intf->printf("Output: %lu; reference: %lu\n",
        (unsigned long) x, (unsigned long) x_ref);
    intf->free(obj);
    return (x == x_ref) ? 1 : 0;
}

MAKE_UINT32_PRNG("JKISS32", run_self_test)
