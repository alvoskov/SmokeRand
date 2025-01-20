/**
 * @file ranrot_bi_shared.c
 * @brief RANROT_BI - a simple non-linear generator, it is not connected to
 * the classical RANROT by Agner Fog.
 * @details Fails Hamming weights based tests from `full` battery, mainly
 * `hamming_ot_values`. Also fails PractRand 0.94 at 16 GiB sample (BCFN test
 * based on Hamming weights).
 *
 * WARNING! The minimal period of RANROT_BI is unknown! This PRNG shouldn't
 * be used in practice!
 *
 * The generator is taken from:
 *
 * - https://github.com/stolendata/ranrot_bi/blob/master/ranrot_bi.h
 *
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
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

static inline uint64_t get_bits_raw(void *state)
{
    RanrotBiState *obj = state;
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
    return (void *) obj;
}

MAKE_UINT64_PRNG("RANROT_BI", NULL)
