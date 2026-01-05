/**
 * @file biski64.c
 * @brief biski64 is a chaotic generator developed by Daniel Cota.
 * @details Its design resembles one round of Feistel network. Biski64
 * passes the `express`, `brief`, `default` and `full` battery but
 * still fails the Hamming weights distribution test (histogram) at large
 * samples:
 *
 * Run 1 (unknown seed)
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
 * Run 2 (the seed is specified)
 *
 * Test configuration:
 *
 *     hamming_distr
 *         test=hamming_distr
 *         nlevels=10
 *         nvalues=549_755_813_888 # 2^39
 *     end 
 *
 * Protocol:
 *
 *    Hamming weights distribution test (histogram)
 *      Sample size, values:     549755813888 (2^39.00 or 10^11.74)
 *      Blocks analysis results
 *            bits |        z          p |    z_xor      p_xor
 *              64 |   -0.190      0.575 |   -0.122      0.548
 *             128 |   -1.784      0.963 |    0.574      0.283
 *             256 |    0.961      0.168 |    9.537   7.35e-22
 *             512 |    0.205      0.419 |    3.172   0.000757
 *            1024 |    0.805      0.211 |    1.265      0.103
 *            2048 |   -0.305       0.62 |   -0.326      0.628
 *            4096 |    0.896      0.185 |    0.300      0.382
 *            8192 |   -0.156      0.562 |   -0.307       0.62
 *           16384 |    0.729      0.233 |   -0.326      0.628
 *           32768 |   -0.184      0.573 |    0.620      0.268
 *      Final: z =   9.537, p = 7.35e-22
 *    
 *    ==================== 'CUSTOM' battery report ====================
 *    Generator name:    biski64
 *    Output size, bits: 64
 *    Used seed:         _01_UcLhv8b1LS4V8pKSVW22yjP7JirNbzRVt4QBfihW/ps=
 *    
 *        # Test name                    xemp              p Interpretation  Thr#
 *    -------------------------------------------------------------------------------
 *        1 hamming_distr             9.53697       7.35e-22 FAIL               0
 *    -------------------------------------------------------------------------------
 *    Passed:        0
 *    Suspicious:    0
 *    Failed:        1
 *    Elapsed time:  00:57:21
 *    Used seed:     _01_UcLhv8b1LS4V8pKSVW22yjP7JirNbzRVt4QBfihW/ps=
 *
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
    obj->mix = rotl64(obj->mix, 16) + rotl64(old_loop_mix, 40);
    obj->ctr += 0x9999999999999999ULL;
    return output;
}

/**
 * @brief The seeding procedure is simplified here (no SplitMix and warmup:
 * SmokeRand can give many quabecause 
 */
static void *create(const CallerAPI *intf)
{
    Biski64State *obj = intf->malloc(sizeof(Biski64State));
    obj->loop_mix = intf->get_seed64();
    obj->mix = intf->get_seed64();
    obj->ctr = intf->get_seed64();
    return obj;
}

/**
 * @brief Based on the reference implementation of the author (Daniel Cota)
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t ref[] = {
        0x2e9dc0924480bb1a, 0x8fd2b3f2f2f047d9, 0x17bbf82c6284b8bd,
        0x9da272374079400f, 0xdf49f285347354a1};
    int is_ok = 1;
    Biski64State *obj = create(intf);
    obj->ctr      = 0x1e9a57bc80e6721d;
    obj->mix      = 0x22118258a9d111a0;
    obj->loop_mix = 0x346edce5f713f8ed;
    for (int i = 0; i < 16; i++) { // Reproduction of the original warmup
        (void) get_bits_raw(obj);  // prodecure.
    }
    for (int i = 0; i < 5; i++) {
        const uint64_t u = get_bits_raw(obj);
        intf->printf("Out: 0x%16.16llX; Ref: 0x%16.16llX\n",
            (unsigned long long) ref[i], (unsigned long long) u);
        if (ref[i] != u) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
}

MAKE_UINT64_PRNG("biski64", run_self_test)
