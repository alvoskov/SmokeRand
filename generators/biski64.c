/**
 * @file biski64.c
 * @brief biski64 is a chaotic generator developed by Daniel Cota.
 * @details Its design resembles one round of Feistel network. Biski64
 * passes the `express`, `brief`, `default` and `full` battery but
 * still fails the Hamming weights distribution test (histogram) at large
 * samples:
 *
 *    Hamming weights distribution test (histogram)
 *      Sample size, values:     274877906944 (2^38.00 or 10^11.44)
 *      Blocks analysis results
 *            bits |        z          p |    z_xor      p_xor
 *              64 |    0.597      0.275 |    1.491      0.068
 *             128 |   -0.631      0.736 |   -1.947      0.974
 *             256 |    0.044      0.482 |    6.010    9.3e-10
 *             512 |    1.308     0.0954 |    2.344    0.00954
 *            1024 |   -0.553       0.71 |   -1.620      0.947
 *            2048 |    0.299      0.382 |   -0.112      0.545
 *            4096 |   -0.317      0.624 |   -1.105      0.865
 *            8192 |   -1.995      0.977 |   -2.291      0.989
 *           16384 |    0.547      0.292 |   -0.777      0.781
 *           32768 |   -1.052      0.854 |    0.573      0.283
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
    uint64_t loop_mix;
    uint64_t mix;
    uint64_t ctr;
} Biski64State;

static inline uint64_t get_bits_raw(void *state)
{
    Biski64State *obj = state;

    uint64_t output = obj->mix + obj->loop_mix;
    uint64_t old_loop_mix = obj->loop_mix;
    obj->loop_mix = obj->ctr ^ obj->mix;
    obj->mix = rotl64(obj->mix, 16) + rotl64(old_loop_mix, 40);
    obj->ctr += 0x9999999999999999;
    return output;
}


static void *create(const CallerAPI *intf)
{
    Biski64State *obj = intf->malloc(sizeof(Biski64State));
    obj->loop_mix = intf->get_seed64();
    obj->mix = intf->get_seed64();
    obj->ctr = intf->get_seed64();
    return obj;
}

MAKE_UINT64_PRNG("biski64", NULL)
