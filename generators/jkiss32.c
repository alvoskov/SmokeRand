/**
 * @file jkiss32.c
 * @brief JKISS is a version of KISS algorithm (2007 version) by J. Marsaglia
 * with paramteres tuned by David Jones.
 * @details It doesn't use multiplication: it is a combination or xorshift32,
 * discrete Weyl sequence and AWC (add with carry) generator. Doesn't require
 * 64-bit integers.
 *
 * References:
 *
 * 1. David Jones, UCL Bioinformatics Group. Good Practice in (Pseudo) Random
 *    Number Generation for Bioinformatics Applications
 *    http://www0.cs.ucl.ac.uk/staff/D.Jones/GoodPracticeRNG.pdf
 * 2. https://groups.google.com/g/comp.lang.fortran/c/5Bi8cFoYwPE
 * 3. https://talkchess.com/viewtopic.php?t=38313&start=10
 *
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
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
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t w;
    uint32_t c;
} JKISS32State;


static inline uint64_t get_bits_raw(void *state)
{

    JKISS32State *obj = state;
    // xorshift part
    obj->y ^= obj->y << 5;
    obj->y ^= obj->y >> 7;
    obj->y ^= obj->y << 22;
    // AWC (add with carry) part
    int32_t t = obj->z + obj->w + obj->c;
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
    JKISS32State *obj = intf->malloc(sizeof(JKISS32State));
    obj->x = 123456789;
    obj->y = 234567891;
    obj->z = 345678912;
    obj->w = 456789123;
    obj->c = 0;
    return (void *) obj;
}

MAKE_UINT32_PRNG("JKISS32", NULL)
