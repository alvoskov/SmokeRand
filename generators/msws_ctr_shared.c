/**
 * @file msws_ctr_shared.c
 * @brief Implementatio of "Middle-Square Weyl Sequence PRNG" 
 * @details Passes SmallCrush, Crush, ??? and ???.
 * References:
 *
 * 1. Bernard Widynski Middle-Square Weyl Sequence RNG
 *    https://arxiv.org/abs/1704.00358

 * @copyright MSWS algorithm is developed by Bernard Widynski.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Middle-square Weyl sequence PRNG state.
 */
typedef struct {
    uint64_t ctr; ///< Counter
    uint64_t key; ///< "Weyl sequence" counter.
} MswsCtrState;


static inline uint64_t get_bits_raw(void *state)
{
    MswsCtrState *obj = state;
    const uint64_t key = obj->key;
    uint64_t t, x, y, z;
    y = x = (obj->ctr++) * key; z = y + key;
    x = x*x + y; x = (x>>32) | (x<<32); // Round 1
    x = x*x + z; x = (x>>32) | (x<<32); // Round 2
    x = x*x + y; x = (x>>32) | (x<<32); // Round 3
    t = x = x*x + z; x = (x>>32) | (x<<32); // Round 4
    return t ^ ((x*x + y) >> 32); // Round 5
}


static void *create(const CallerAPI *intf)
{
    MswsCtrState *obj = intf->malloc(sizeof(MswsCtrState));
    obj->ctr = intf->get_seed64();
    obj->key = 0xb5ad4eceda1ce2a9;
    return (void *) obj;
}


MAKE_UINT64_PRNG("MswsCtr", NULL)
