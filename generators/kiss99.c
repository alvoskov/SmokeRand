/**
 * @file kiss99.c
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
 * NOTE: it intentionally reproduces an error from the original code
 * of G. Marsaglia, SHR constants are (17, 13, 5), should be (13, 17, 5).
 * This error has been detected by Greg Rose.
 *
 * References:
 *
 * - https://groups.google.com/group/sci.stat.math/msg/b555f463a2959bb7/
 * - http://www0.cs.ucl.ac.uk/staff/d.jones/GoodPracticeRNG.pdf
 * - Greg Rose. KISS: A Bit Too Simple. 2011. https://eprint.iacr.org/2011/007.pdf
 * - Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *   V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 *
 * @copyright The KISS99 algorithm is developed by George Marsaglia.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
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


static inline uint64_t get_bits_raw(void *state)
{
    KISS99State *obj = state;
    // LCG generator
    obj->jcong = 69069u * obj->jcong + 1234567u;
    // MWC generators
    obj->z = 36969u * (obj->z & 0xFFFF) + (obj->z >> 16);
    obj->w = 18000u * (obj->w & 0xFFFF) + (obj->w >> 16);
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
    obj->z = (uint32_t) (seed0 & 0xFFFF) | 0x10000; // MWC generator 1: prevent bad seeds
    obj->w = (uint32_t) ((seed0 >> 16) & 0xFFFF) | 0x10000; // MWC generator 2: prevent bad seeds
    obj->jsr = (uint32_t) (seed1 >> 32) | 0x1; // SHR3 mustn't be init with 0
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
        val = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("Reference value: %u\n", refval);
    intf->printf("Obtained value:  %u\n", val);
    intf->printf("Difference:      %u\n", refval - val);
    return refval == val;
}


MAKE_UINT32_PRNG("KISS99", run_self_test)
