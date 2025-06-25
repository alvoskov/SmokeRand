/**
 * @file mtc32.c
 * @brief A very fast multiplication-based chaotic PRNG by Chris Doty-Humphrey.
 * @details References:
 *
 * 1. https://sourceforge.net/p/pracrand/discussion/366935/thread/f310c67275/
 *
 * @copyright MTC32 algorithm was developed by Chris Doty-Humphrey,
 * the author of PractRand.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
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
    uint32_t a;
    uint32_t b;
    uint32_t ctr;
} Mtc32State;


static inline uint64_t get_bits_raw(void *state)
{
    Mtc32State *obj = state;
    uint32_t old = obj->a + obj->b;
    obj->a = (obj->b * 1566083941u) ^ ++obj->ctr;
    obj->b = rotl32(old, 13);
    return obj->a;
}


static void *create(const CallerAPI *intf)
{
    Mtc32State *obj = intf->malloc(sizeof(Mtc32State));
    obj->a = intf->get_seed32();
    obj->b = intf->get_seed32();
    obj->ctr = intf->get_seed32();
    return (void *) obj;
}


MAKE_UINT32_PRNG("Mtc32", NULL)
