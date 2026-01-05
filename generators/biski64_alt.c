/**
 * @file biski64_alt.c
 * @brief biski64_alt is a chaotic generator developed by Daniel Cota
 * modified by A.L. Voskov.
 * @details Its design resembles one round of Feistel network. Biski64
 * passes the `express`, `brief`, `default` and `full` battery. A minor
 * modification by A.L.Voskov allows it to passs the Hamming weights
 * distribution test (histogram) at large samples:
 *
 *    Hamming weights distribution test (histogram)
 *      Sample size, values:     274877906944 (2^38.00 or 10^11.44)
 *      Blocks analysis results
 *            bits |        z          p |    z_xor      p_xor
 *              64 |   -1.765      0.961 |   -1.302      0.903
 *             128 |   -0.840        0.8 |   -0.349      0.636
 *             256 |   -0.681      0.752 |    0.196      0.422
 *             512 |    0.491      0.312 |   -0.841        0.8
 *            1024 |   -0.764      0.778 |    1.257      0.104
 *            2048 |   -0.695      0.756 |    1.178      0.119
 *            4096 |    0.878       0.19 |    0.249      0.401
 *            8192 |   -0.201       0.58 |    0.335      0.369
 *           16384 |   -0.927      0.823 |    1.266      0.103
 *           32768 |   -1.086      0.861 |    0.838      0.201
 *      Final: z =  -1.765, p = 0.961
 *
 * However, it slightly slows it down.
 *
 * References:
 * 1. https://github.com/danielcota/biski64
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
    uint64_t loop_mix;
    uint64_t mix;
    uint64_t ctr;
} Biski64State;


static inline uint64_t get_bits_raw(Biski64State *obj)
{
    const uint64_t output = obj->mix + obj->loop_mix;
    const uint64_t old_loop_mix = obj->loop_mix;
    obj->loop_mix = obj->ctr ^ obj->mix;
    obj->mix = (obj->mix ^ rotl64(obj->mix, 16)) + rotl64(old_loop_mix, 40);
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

MAKE_UINT64_PRNG("biski64_alt", NULL)
