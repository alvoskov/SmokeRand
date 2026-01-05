/**
 * @file cmwc4096.c
 * @brief CMWC4096 ("Mother-of-All") PRNG implementation.
 * It has good statistical properties, huge period and high performance.
 * @details The CMWC4096 algorithm is developed by G.Marsaglia:
 *
 * - George Marsaglia. Random Number Generators // Journal of Modern Applied
 *   Statistical Methods. 2003. V. 2. N 1. P. 2-13.
 *   https://doi.org/10.22237/jmasm/1051747320
 *
 * @copyright Based on scientific publication by G.Marsaglia.
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

typedef struct {
    uint32_t Q[4096];
    uint32_t c;
    uint32_t i;
} Cmwc4096State;

static inline uint64_t get_bits_raw(Cmwc4096State *obj)
{
    const uint64_t a = 18782ull;
    obj->i = (obj->i + 1) & 4095;
    const uint64_t t = a * obj->Q[obj->i] + obj->c;
    obj->c = (uint32_t) (t >> 32);
    uint32_t x = (uint32_t) (t + obj->c);
    if (x < obj->c) {
        x++;
        obj->c++;
    }
    return obj->Q[obj->i] = 0xfffffffe - x;
}

static void *create(const CallerAPI *intf)
{
    Cmwc4096State *obj = intf->malloc(sizeof(Cmwc4096State));
    uint64_t seed = intf->get_seed64();
    uint32_t state = (uint32_t) (seed >> 32);
    obj->Q[0] = (uint32_t) seed;
    for (size_t i = 1; i < 4096; i++) {
        state = 69069u * state + 1u;
        obj->Q[i] = state;
    }
    obj->c = 123;
    obj->i = 4095;
    return (void *) obj;
}

MAKE_UINT32_PRNG("CMWC4096", NULL)
