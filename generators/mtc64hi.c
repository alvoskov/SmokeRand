/**
 * @file mtc64hi.c
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
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Middle-square Weyl sequence PRNG state.
 */
typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t ctr;
} Mtc64HiState;


static inline uint64_t get_bits_raw(Mtc64HiState *obj)
{
    uint64_t hi, old;
    old = unsigned_mul128(obj->a, 0x9e3779b97f4a7c15ull, &hi);
    obj->a = hi ^ obj->b;
    obj->b = old + ++obj->ctr;
    return obj->a;
}


static void *create(const CallerAPI *intf)
{
    Mtc64HiState *obj = intf->malloc(sizeof(Mtc64HiState));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->ctr = intf->get_seed64();
    return obj;
}


MAKE_UINT64_PRNG("Mtc64Hi", NULL)
