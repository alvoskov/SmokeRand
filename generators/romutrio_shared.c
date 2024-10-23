/**
 * @file romutrio_shared.c
 * @brief Implementation of RomuTrio PRNG developed by Mark A. Overton.
 * It passes `brief`, 'default` and `full` batteries of SmokeRand,
 * it also passes BigCrush and PractRand.
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
 * @copyright RomuTrio algorithm is developed by Mark Overton.
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
 * @brief RomuTrio state
 */
typedef struct {
    uint64_t x;
    uint64_t y;
    uint64_t z;
} RomuTrioState;


static inline uint64_t rotl(uint64_t x, uint64_t r)
{
    return (x << r) | (x >> (64 - r));
}


static inline uint64_t get_bits_raw(void *state)
{
    RomuTrioState *obj = state;
    uint64_t x = obj->x, y = obj->y, z = obj->z;
    obj->x = 15241094284759029579ull * z;
    obj->y = rotl(y - x, 12);
    obj->z = rotl(z - y, 44);
    return x;
}

static void *create(const CallerAPI *intf)
{
    RomuTrioState *obj = intf->malloc(sizeof(RomuTrioState));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    do {
        obj->z = intf->get_seed64();
    } while (obj->z == 0);
    return (void *) obj;
}

MAKE_UINT64_PRNG("RomuTrio", NULL)
