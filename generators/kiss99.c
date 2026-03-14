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
 * NOTE: the two versions are implemented here:
 * - `corrected`, the default one that uses the (13, 17, 5) in the xorshift32
 *   part, i.e. it corrects the type in the original Marsaglia post.
 * - `original`, intentionally reproduces an error from the original code
 *    of G. Marsaglia, SHR constants are (17, 13, 5), should be (13, 17, 5).
 *    This error has been detected by Greg Rose.
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

#define KISS99_TEMPLATE(name, a, b, c) \
static inline uint64_t get_bits_##name##_raw(KISS99State *obj) \
{ \
    obj->jcong = 69069u * obj->jcong + 1234567u; /* LCG part */ \
    obj->z = 36969u * (obj->z & 0xFFFF) + (obj->z >> 16); /* MWC part */ \
    obj->w = 18000u * (obj->w & 0xFFFF) + (obj->w >> 16); \
    const uint32_t mwc = (obj->z << 16) + obj->w; \
    obj->jsr ^= obj->jsr << (a);  /* xorshift32 part */ \
    obj->jsr ^= obj->jsr >> (b); \
    obj->jsr ^= obj->jsr << (c); \
    return (mwc ^ obj->jcong) + obj->jsr; /* Output */ \
}

/**
 * @brief The original version from Marsaglia post that uses the [17,13,5]
 * version of xorshift32. It seems that it was a type that reduced the period
 * of the generator.
 */
KISS99_TEMPLATE(original, 17, 13, 5)
MAKE_GET_BITS_WRAPPERS(original)

/**
 * @brief The corrected version of KISS99 that uses [13,17,5] triple in
 * the xorshift32 part instead of the original [17,13,5] one. It gives
 * the full period to the LFSR part.
 */
KISS99_TEMPLATE(corrected, 13, 17, 5)
MAKE_GET_BITS_WRAPPERS(corrected)

static void *create(const CallerAPI *intf)
{
    KISS99State *obj = intf->malloc(sizeof(KISS99State));
    uint64_t seed0 = intf->get_seed64(); // For MWC
    uint64_t seed1 = intf->get_seed64(); // For SHR3 and LCG
    obj->z = (uint32_t) (seed0 & 0xFFFF) | 0x10000; // MWC generator 1: prevent bad seeds
    obj->w = (uint32_t) ((seed0 >> 16) & 0xFFFF) | 0x10000; // MWC generator 2: prevent bad seeds
    obj->jsr = (uint32_t) (seed1 >> 32) | 0x1; // SHR3 mustn't be init with 0
    obj->jcong = (uint32_t) seed1; // LCG accepts any seed
    return obj;
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
        val = (uint32_t) get_bits_original_raw(&obj);
    }
    intf->printf("Reference value: %u\n", refval);
    intf->printf("Obtained value:  %u\n", val);
    intf->printf("Difference:      %u\n", refval - val);
    return refval == val;
}


static const GeneratorParamVariant gen_list[] = {
    {"",          "KISS99:corrected", 32, default_create, get_bits_corrected, get_sum_corrected},
    {"corrected", "KISS99:corrected", 32, default_create, get_bits_corrected, get_sum_corrected},
    {"original",  "KISS99:original",  32, default_create, get_bits_original,  get_sum_original},
    GENERATOR_PARAM_VARIANT_EMPTY
};

static const char description[] =
"KISS99 is the famous combined generator designed by G. Marsaglia.\n"
"The next param values are supported:\n"
"  corrected - uses the [13,17,5] triple in the xorshift32 part (default)\n"
"              It gives the full period\n"
"  original  - uses the [17,13,5] triple in the xorshift32 part that\n"
"              corresponds to the original Marsaglia post with the typo\n";

int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = run_self_test;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
