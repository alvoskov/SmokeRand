/**
 * @file jkiss.c
 * @brief JKISS pseudorandom number generator was suggested by David Jones,
 * is a modification of KISS99 algorithm by George Marsaglia.
 * @details Uses one MWC generator with \f$ b = 2^{32} \f$ instead of two 
 * MWC generators with \f$ b = 2^{64} \f$. It does require 64-bit integers
 * in the programming language (C99) but is friendly even to 32-bit CPUs
 * like Intel 80386.
 *
 * References:
 *
 * 1. David Jones, UCL Bioinformatics Group. Good Practice in (Pseudo) Random
 *    Number Generation for Bioinformatics Applications
 *    http://www0.cs.ucl.ac.uk/staff/D.Jones/GoodPracticeRNG.pdf
 * 2. https://groups.google.com/group/sci.stat.math/msg/b555f463a2959bb7/
 *
 * http://www0.cs.ucl.ac.uk/staff/D.Jones/GoodPracticeRNG.pdf
 *
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 *
 * The KISS99 algorithm is developed by George Marsaglia, its JKISS modification
 * was suggested by David Jones.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief JKISS PRNG state.
 */
typedef struct {
    uint32_t x; ///< 32-bit LCG state
    uint32_t y; ///< xorshift32 state
    uint32_t z; ///< MWC state: lower part
    uint32_t c; ///< MWC state: higher part (carry)
} JKISSState;


static inline uint64_t get_bits_raw(void *state)
{
    JKISSState *obj = state;
    // LCG part
    obj->x = 314527869u * obj->x + 1234567u;
    // xorshift part
    obj->y ^= obj->y << 5;
    obj->y ^= obj->y >> 7;
    obj->y ^= obj->y << 22;
    // MWC part
    uint64_t t = 4294584393ULL * obj->z + obj->c;
    obj->c = (uint32_t) (t >> 32);
    obj->z = (uint32_t) t;
    // Combined output
    return obj->x + obj->y + obj->z;
}

static void *create(const CallerAPI *intf)
{
    JKISSState *obj = intf->malloc(sizeof(JKISSState));
    uint64_t s0 = intf->get_seed64();
    obj->x = (uint32_t) (s0 >> 32);
    obj->y = (uint32_t) (s0 & 0xFFFFFFFF) | 0x1;
    obj->z = ~s0;
    obj->c = 1;
    return (void *) obj;
}

MAKE_UINT32_PRNG("JKISS", NULL)
