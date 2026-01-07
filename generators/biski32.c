/**
 * @file biski32.c
 * @brief biski32 is a chaotic generator developed by Daniel Cota.
 * @details Its design resembles one round of Feistel network. It has
 * two version: `v2` (default) that passes based on the suggestion
 * from [2] and the `v1` (the original version) from [1] that 
 * fails the Hamming weights distribution test (histogram) at large
 * samples (`hamming_distr`):
 *
 *    Hamming weights distribution test (histogram)
 *      Sample size, values:     137438953472 (2^37.00 or 10^11.14)
 *      Blocks analysis results
 *            bits |        z          p |    z_xor      p_xor
 *              32 |   -0.400      0.656 |   -0.080      0.532
 *              64 |    1.165      0.122 |   -0.754      0.775
 *             128 |   -0.396      0.654 |    8.256   7.54e-17
 *             256 |   -0.711      0.762 |    4.613   1.99e-06
 *             512 |    0.625      0.266 |   -0.012      0.505
 *            1024 |    0.936      0.175 |    2.203     0.0138
 *            2048 |   -0.429      0.666 |    0.611      0.271
 *            4096 |   -1.041      0.851 |   -1.367      0.914
 *            8192 |   -1.407       0.92 |   -0.811      0.791
 *           16384 |    0.457      0.324 |   -0.099      0.539
 *      Final: z =   8.256, p = 7.54e-17
 *
 * References:
 * 1. https://github.com/danielcota/biski64
 * 2. https://www.reddit.com/r/RNG/comments/1ptyn5c/comment/nxoh0y7/
 *
 *
 * @copyright
 * (c) 2025 Daniel Cota (https://github.com/danielcota/biski64)
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t loop_mix;
    uint32_t mix;
    uint32_t ctr;
} Biski32State;

///////////////////////////////////////////////////
///// The newer version with optimized shifts /////
///////////////////////////////////////////////////

static inline uint64_t get_bits_ver2_raw(Biski32State *obj)
{
    const uint32_t output = obj->mix + obj->loop_mix;
    const uint32_t old_loop_mix = obj->loop_mix;
    obj->loop_mix = obj->ctr ^ obj->mix;
    obj->mix = rotl32(obj->mix, 7) + rotl32(old_loop_mix, 19);
    obj->ctr += 0x99999999;
    return output;
}

MAKE_GET_BITS_WRAPPERS(ver2)

/////////////////////////////////////////////////////////////////
///// The initial version that fails the hamming_distr test /////
/////////////////////////////////////////////////////////////////

static inline uint64_t get_bits_ver1_raw(Biski32State *obj)
{
    const uint32_t output = obj->mix + obj->loop_mix;
    const uint32_t old_loop_mix = obj->loop_mix;
    obj->loop_mix = obj->ctr ^ obj->mix;
    obj->mix = rotl32(obj->mix, 8) + rotl32(old_loop_mix, 20);
    obj->ctr += 0x99999999;
    return output;
}

MAKE_GET_BITS_WRAPPERS(ver1)

/////////////////////////////////////////
///// Interfaces and initialization /////
/////////////////////////////////////////

static void *create(const CallerAPI *intf)
{
    Biski32State *obj = intf->malloc(sizeof(Biski32State));
    obj->loop_mix = intf->get_seed32();
    obj->mix = intf->get_seed32();
    obj->ctr = intf->get_seed32();
    return obj;
}


static const GeneratorParamVariant gen_list[] = {
    {"",   "biski32:v2", 32, default_create, get_bits_ver2, get_sum_ver2},
    {"v2", "biski32:v2", 32, default_create, get_bits_ver2, get_sum_ver2},
    {"v1", "biski32:v1", 32, default_create, get_bits_ver1, get_sum_ver1},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"biski32 is a chaotic PRNG with a linear part developed by Daniel Cota.\n"
"The next param values are supported:\n"
"  v2 - the updated version with improved quality (default)\n"
"  v1 - the original version that fails the hamming_distr test\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = NULL;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
