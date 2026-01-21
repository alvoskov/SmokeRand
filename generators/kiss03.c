/**
 * @file kiss03.c
 * @brief KISS pseudorandom number generator was suggested by George Marsaglia.
 * @details Uses one MWC generator with \f$ b = 2^{32} \f$ instead of two 
 * MWC generators with \f$ b = 2^{64} \f$ in KISS99. It does require 64-bit
 * integers in the programming language (C99) but is friendly even to 32-bit
 * CPUs like Intel 80386.
 *
 * The later modificaton is JKISS by D. Jones with improved constants for LCG
 * and MWC generators.
 *
 * Spectral test results for MWC:
 *
 * - Marsaglia: 0.5111; 0.0004; 0.4002; 1.1841; 0.6442; 0.8542; 5.0674;
 * - Jones:     3.1413; 0       3.0008; 0.1032; 1.6091; 1.8168; 0.6183;
 *
 * Spectral test results for LCG:
 *
 * - Marsaglia:
 * - Jones:     
 *     
 *
 * References:
 *
 * 1. George Marsaglia. Random Number Generators // Journal of Modern Applied
 *    Statistical Methods. 2003. V. 2. N 1. P. 2-13.
 *    https://doi.org/10.22237/jmasm/1051747320
 * 2. David Jones, UCL Bioinformatics Group. Good Practice in (Pseudo) Random
 *    Number Generation for Bioinformatics Applications
 *    http://www0.cs.ucl.ac.uk/staff/D.Jones/GoodPracticeRNG.pdf
 * 3. https://groups.google.com/group/sci.stat.math/msg/b555f463a2959bb7/
 * 4. https://cmcmsu.info/1course/random.generators.algs.htm
 *
 * @copyright The KISS03 algorithm is developed by George Marsaglia.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Kiss03 PRNG state.
 */
typedef struct {
    uint32_t x; ///< 32-bit LCG state
    uint32_t y; ///< xorshift32 state
    uint32_t z; ///< MWC state: lower part
    uint32_t c; ///< MWC state: higher part (carry)
} Kiss03State;


static inline uint64_t get_bits_raw(Kiss03State *obj)
{
    // LCG part
    obj->x = 69069U * obj->x + 12345U;
    // xorshift part
    obj->y ^= obj->y << 13;
    obj->y ^= obj->y >> 17;
    obj->y ^= obj->y << 5;
    // MWC part
    const uint64_t t = 698769069ULL * obj->z + obj->c;
    obj->c = (uint32_t) (t >> 32);
    obj->z = (uint32_t) t;
    // Combined output
    return obj->x + obj->y + obj->z;
}


static void *create(const CallerAPI *intf)
{
    Kiss03State *obj = intf->malloc(sizeof(Kiss03State));
    seed64_to_2x32(intf, &obj->x, &obj->y);
    seed64_to_2x32(intf, &obj->z, &obj->c);
    if (obj->y == 0) {
        obj->y = 0x12345678;
    }
    obj->c = (obj->c & 0xFFFFFFF) + 1;
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t x_ref = 0x8E41D4F8;
    uint32_t x;
    Kiss03State *obj = intf->malloc(sizeof(Kiss03State));
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


MAKE_UINT32_PRNG("KISS2003", run_self_test)
