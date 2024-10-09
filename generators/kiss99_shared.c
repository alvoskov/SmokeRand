/**
 * @file kiss99_shared.c
 * @brief KISS99 pseudorandom number generator by George Marsaglia.
 * It passes SmallCrush, Crush and BigCrush batteries, has period about 2^123
 * and doesn't require 64-bit arithmetics.
 * @details Description by George Marsaglia:
 *
 * The KISS generator, (Keep It Simple Stupid), is designed to combine
 * the two multiply-with-carry generators in MWC with the 3-shift register
 * SHR3 and the congruential generator CONG, using addition and exclusive-or.
 * Period about 2^123. It is one of my favorite generators.
 * 
 * References:
 *
 * - https://groups.google.com/group/sci.stat.math/msg/b555f463a2959bb7/
 * - http://www0.cs.ucl.ac.uk/staff/d.jones/GoodPracticeRNG.pdf
 *
 * @copyright The KISS99 algorithm is developed by George Marsaglia.
 *
 * Implementation for TestU01-threads:
 *
 * (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * 
 *
 * All rights reserved.
 *
 * This software is provided under the Apache 2 License.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief KISS99 PRNG state.
 * @details Contains states of 3 PRNG: LCG, SHR3, MWC.
 * There are three rules:
 * - z and w are initialized as MWC generators
 * - jsr mustn't be initialized to 0
 */
typedef struct {
    uint32_t z;     ///< MWC state 1: c - upper half, x - lower half
    uint32_t w;     ///< MWC state 2: c - upper half, x - lower half
    uint32_t jsr;   ///< SHR3 state
    uint32_t jcong; ///< LCG state
} KISS99State;


static uint64_t get_bits(void *state)
{
    KISS99State *obj = state;
    // LCG generator
    obj->jcong = 69069 * obj->jcong + 1234567;
    // MWC generators
    obj->z = 36969 * (obj->z & 0xFFFF) + (obj->z >> 16);
    obj->w = 18000 * (obj->w & 0xFFFF) + (obj->w >> 16);
    uint32_t mwc = (obj->z << 16) + obj->w;
    // SHR3 generator
    uint32_t jsr = obj->jsr;
    jsr ^= (jsr << 17);
    jsr ^= (jsr >> 13);
    jsr ^= (jsr << 5);
    obj->jsr = jsr;
    // Output (combination of generators)
    uint32_t u = (mwc ^ obj->jcong) + jsr;
    return u;
}


static void *create(const CallerAPI *intf)
{
    KISS99State *obj = intf->malloc(sizeof(KISS99State));
    uint64_t seed0 = intf->get_seed64(); // For MWC
    uint64_t seed1 = intf->get_seed64(); // For SHR3 and LCG
    obj->z = (seed0 & 0xFFFF) | 0x10000; // MWC generator 1: prevent bad seeds
    obj->w = ((seed0 >> 16) & 0xFFFF) | 0x10000; // MWC generator 2: prevent bad seeds
    obj->jsr = (seed1 >> 32) | 0x1; // SHR3 mustn't be init with 0
    obj->jcong = (uint32_t) seed1; // LCG accepts any seed
    return (void *) obj;
}


/**
 * @brief An internal self-test, taken from Marsaglia post.
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint32_t refval = 1372460312U;
    uint32_t val = 0;
    KISS99State obj;
    obj.z   = 12345; obj.w     = 65435;
    obj.jsr = 34221; obj.jcong = 12345; 
    for (long i = 1; i < 1000001 + 256; i++) {
        val = get_bits(&obj);
    }
    intf->printf("Reference value: %u\n", refval);
    intf->printf("Obtained value:  %u\n", val);
    intf->printf("Difference:      %u\n", refval - val);
    return refval == val;
}


MAKE_UINT32_PRNG("KISS99", run_self_test)
