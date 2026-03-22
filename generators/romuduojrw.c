/**
 * @file romuduojrw.c
 * @brief A modification of RomuDuoJr PRNG with the counter ("discrete Weyl
 * sequence").
 * @details Its exact period is unknown but cannot be lower than \f$2^{64}\f$,
 * the average period is likely close to \f$2^{191}\f$.
 *
 * References:
 *
 * 1. Mark A. Overton. Romu: Fast Nonlinear Pseudo-Random Number Generators
 *    Providing High Quality. https://doi.org/10.48550/arXiv.2002.11331
 * 2. Discussion of Romu: https://news.ycombinator.com/item?id=22447848
 *
 * @copyright RomuDuoJr algorithm was developed by Mark Overton,
 * the modification with counter was suggested by A.L. Voskov.
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
 * @brief RomuDuoJr-Weyl state
 */
typedef struct {
    uint64_t x;
    uint64_t y;
    uint64_t w;
} RomuDuoJrWeylState;


static inline uint64_t get_bits_raw(RomuDuoJrWeylState *obj)
{
    const uint64_t x = obj->x;
    obj->x = 15241094284759029579ull * (obj->y + obj->w++);
    obj->y = rotl64(obj->y - x, 27);
    return x;
}


static void *create(const CallerAPI *intf)
{
    RomuDuoJrWeylState *obj = intf->malloc(sizeof(RomuDuoJrWeylState));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->w = intf->get_seed64();
    return obj;
}


MAKE_UINT64_PRNG("RomuDuoJr-Weyl", NULL)
