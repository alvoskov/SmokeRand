/**
 * @file ranrot_bi.c
 * @brief RANROT_BI - a simple non-linear generator, it is not connected to
 * the classical RANROT by Agner Fog.
 * @details Fails `default` and `full` batteries:
 *
 * - `default`: Hamming weights based tests `hamming_distr`.
 * - `full` : also `hamming_ot_values`.
 *
 * This PRNG also fails PractRand 0.94 at 16 GiB sample (BCFN test based on
 * Hamming weights).
 *
 * WARNING! It has no guaranteed minimal period, bad seeds are theoretically
 * possible. Usage of this generator for statistical, scientific and
 * engineering computations is strongly discouraged!
 *
 * However this generator is interesting for checking tests that are aimed to
 * find biases in Hamming weights.
 *
 * The generator is taken from:
 *
 * - https://github.com/stolendata/ranrot_bi/blob/master/ranrot_bi.h
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t hi;
    uint64_t lo;
} RanrotBiState;

static inline uint64_t get_bits_raw(RanrotBiState *obj)
{
    obj->hi = ( obj->hi << 19 ) + ( obj->hi >> 23 );
    obj->lo = ( obj->lo << 29 ) + ( obj->lo >> 31 );

    obj->hi += obj->lo;
    obj->lo += obj->hi;

    return obj->hi;
}

static void *create(const CallerAPI *intf)
{
    RanrotBiState *obj = intf->malloc(sizeof(RanrotBiState));
    obj->lo = intf->get_seed64();
    obj->hi = ~obj->lo; // Replacement to random seed doesn't improve PRNG
    return obj;
}

MAKE_UINT64_PRNG("RANROT_BI", NULL)
