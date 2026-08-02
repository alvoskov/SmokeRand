/**
 * @file thurst.c
 * @brief Thurst generator based on scrambling of "discrete Weyl sequence".
 * @detail It resembles SplitMix64 but uses an even increment in the discrete
 * weyl sequence and non-bijective output function. It does fail the
 * `hamming_distr` test (its part with pairwise XORs)
 *
 * References:
 *
 * - https://gist.github.com/tommyettinger/e6d3e8816da79b45bfe582384c2fe14a
 *
 * @copyright The thurst PRNG was developed by Tommy Ettinger. The
 * `v2` modification was made by A.L. Voskov to make it pass the `hamming_distr`
 * test. But `v2` fails the BRank test in PractRand 0.96 at 2 TiB.
 *
 * Reentrant C99 implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

////////////////////////////////////////////////////////
///// The original version (fails `hamming_distr`) /////
////////////////////////////////////////////////////////

static inline uint64_t get_bits_v1_raw(Lcg64State *obj)
{
	const uint64_t s = obj->x;
	const uint64_t z = (s ^ (s >> 25)) * (obj->x += 0x6A5D39EAE12657AAU);
	return z ^ (z >> 22);
}

MAKE_GET_BITS_WRAPPERS(v1)


///////////////////////////////////////////////////////////////
///// The modified version (doesn't fail `hamming_distr`) /////
///////////////////////////////////////////////////////////////

static inline uint64_t get_bits_v2_raw(Lcg64State *obj)
{
	const uint64_t s = obj->x;
	const uint64_t z = (s ^ (s >> 25)) * (obj->x += 0x6A5D39EAE12657AAU);
	return z ^ rotl64(z, 13) ^ rotl64(z, 47); // from xorrot64
}

MAKE_GET_BITS_WRAPPERS(v2)


//////////////////////
///// Interfaces /////
//////////////////////

static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64() | 0x1U; // Seed must be odd!
    return obj;
}

static const GeneratorParamVariant gen_list[] = {
    {"",   "thurst:v1", 64, default_create, get_bits_v1, get_sum_v1},
    {"v1", "thurst:v1", 64, default_create, get_bits_v1, get_sum_v1},
    {"v2", "thurst:v2", 64, default_create, get_bits_v2, get_sum_v2},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"The thurst PRNG is based on a scrambled Weyl sequence and has\n"
"a period around 2^63.\n"
"The next param values are supported:\n"
"  v1 - the original PRNG by Tommy Ettinger (default version)\n"
"  v2 - the improved version by A.L. Voskov\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = NULL;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
