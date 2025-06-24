/**
 * @file mtc64.c
 * @brief A very fast multiplication-based chaotic PRNG by Chris Doty-Humphrey.
 * @details References:
 *
 * 1. https://sourceforge.net/p/pracrand/discussion/366935/thread/f310c67275/
 *
 * @copyright MTC64 algorithm was developed by Chris Doty-Humphrey,
 * the author of PractRand.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
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
    uint64_t a;
    uint64_t b;
    uint64_t ctr;
} Mtc64State;


static inline uint64_t get_bits_raw(void *state)
{
    Mtc64State *obj = state;
    uint64_t old = obj->a + obj->b;
    obj->a = (obj->b * 0x9e3779b97f4a7c15ull) ^ ++obj->ctr;
    obj->b = rotl64(old, 23);
    return obj->a;
}


static void *create(const CallerAPI *intf)
{
    Mtc64State *obj = intf->malloc(sizeof(Mtc64State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->ctr = intf->get_seed64();
    return (void *) obj;
}


MAKE_UINT64_PRNG("Mtc64", NULL)
