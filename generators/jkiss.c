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
 * 3. George Marsaglia. Random Number Generators // Journal of Modern Applied
 *    Statistical Methods. 2003. V. 2. N 1. P. 2-13.
 *    https://doi.org/10.22237/jmasm/1051747320
 *
 * @copyright
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 *
 * The KISS03 algorithm is developed by George Marsaglia, its JKISS modification
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


static inline uint64_t get_bits_raw(JKISSState *obj)
{
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
    seed64_to_2x32(intf, &obj->x, &obj->y);
    seed64_to_2x32(intf, &obj->z, &obj->c);
    if (obj->y == 0) {
        obj->y = 0x12345678;
    }
    obj->c = (obj->c & 0x7FFFFFFF) + 1;
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t x_ref = 3388360461;
    uint32_t x;
    JKISSState *obj = intf->malloc(sizeof(JKISSState));
    obj->x = 123456789; obj->y = 987654321;
    obj->z = 43219876;  obj->c = 6543217;
    for (long i = 0; i < 10000000; i++) {
        x = (uint32_t) get_bits_raw(obj);
    }
    intf->printf("Observed: 0x%.8lX; expected: 0x%.8lX\n",
        (unsigned long) x, (unsigned long) x_ref);
    intf->free(obj);
    return (x == x_ref) ? 1 : 0;
}


MAKE_UINT32_PRNG("JKISS", run_self_test)
