/**
 * @file biski32.c
 * @brief biski32 is a chaotic generator developed by Daniel Cota.
 * @details Its design resembles one round of Feistel network. Biski32
 * passes the `express`, `brief`, `default` and `full` batteries but
 * still fails the Hamming weights distribution test (histogram) at large
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
 *
 *
 * @copyright
 * (c) 2025 Daniel Cota (https://github.com/danielcota/biski64)
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
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

static inline uint64_t get_bits_raw(void *state)
{
    Biski32State *obj = state;

    uint32_t output = obj->mix + obj->loop_mix;
    uint32_t old_loop_mix = obj->loop_mix;
    obj->loop_mix = obj->ctr ^ obj->mix;
    obj->mix = rotl32(obj->mix, 8) + rotl32(old_loop_mix, 20);
    obj->ctr += 0x99999999;
    return output;
}


static void *create(const CallerAPI *intf)
{
    Biski32State *obj = intf->malloc(sizeof(Biski32State));
    obj->loop_mix = intf->get_seed32();
    obj->mix = intf->get_seed32();
    obj->ctr = intf->get_seed32();
    return obj;
}

MAKE_UINT32_PRNG("biski32", NULL)
