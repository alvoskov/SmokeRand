/**
 * @file romuduojr.c
 * @brief Implementation of RomuDuoJr PRNG developed by Mark A. Overton.
 *
 * WARNING! IT HAS NO GUARANTEED MINIMAL PERIOD! BAD SEEDS ARE POSSIBLE!
 * DON'T USE THIS PRNG FOR ANY SERIOUS WORK!
 *
 * References:
 *
 * 1. Mark A. Overton. Romu: Fast Nonlinear Pseudo-Random Number Generators
 *    Providing High Quality. https://doi.org/10.48550/arXiv.2002.11331
 * 2. Discussion of Romu: https://news.ycombinator.com/item?id=22447848
 *
 * @copyright RomuDuoJr algorithm is developed by Mark Overton.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief RomuDuoJr state
 */
typedef struct {
    uint64_t x;
    uint64_t y;
} RomuDuoJrState;


static inline uint64_t get_bits_raw(RomuDuoJrState *obj)
{
    const uint64_t x = obj->x;
    obj->x = 15241094284759029579ull * obj->y;
    obj->y = rotl64(obj->y - x, 27);
    return x;
}


static void *create(const CallerAPI *intf)
{
    RomuDuoJrState *obj = intf->malloc(sizeof(RomuDuoJrState));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    if (obj->x == 0 && obj->y == 0) {
        obj->x = 0xDEADBEEF;
        obj->y = 0xCAFEBABE;
    }
    return obj;
}


MAKE_UINT64_PRNG("RomuDuoJr", NULL)
