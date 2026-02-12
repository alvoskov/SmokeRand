/**
 * @file combmrg96.c
 * @brief
 * @details
 *
 * References:
 *
 * 1. Pierre L'Ecuyer. Combined Multiple Recursive Random Number Generators //
 *    Operations Research. 1996. V.44. N 5. P.816-822.
 *    https://doi.org/10.1287/opre.44.5.816
 *
 * @copyright The original algorithm was suggested by P.L'Ecuyer.
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    int32_t x[3]; ///< (x_{n-1}, x_{n-2}, x_{n-3})
    int32_t y[3]; ///< (y_{n-1}, y_{n-2}, y_{n-3})
} CombMrg96State;


static const int64_t a2 = 63308, a3 = -183326, m1 = 2147483647,
                     b2 = 86098, b3 = -539608, m2 = 2145483479;


static uint64_t get_bits_raw(CombMrg96State *obj)
{
    int32_t *x = obj->x, *y = obj->y;
    // Component 1
    int64_t p1 = (a2*x[1] + a3*x[2]) % m1;
    if (p1 < 0) p1 += m1;
    x[2] = x[1]; x[1] = x[0]; x[0] = (int32_t) p1;
    // Component 2
    int64_t p2 = (b2*y[1] + b3*y[2]) % m2;
    if (p2 < 0) p2 += m1;
    y[2] = y[1]; y[1] = y[0]; y[0] = (int32_t) p2;
    // Output function
    const int32_t out = (y[0] < y[1]) ? (y[0] - y[1] + m1) : (y[0] - y[1]);
    return (uint32_t)out << 1;    
}


static void *create(const CallerAPI *intf)
{
    CombMrg96State *obj = intf->malloc(sizeof(CombMrg96State));
    for (int i = 0; i < 3; i++) {
        obj->x[i] = (int32_t) (intf->get_seed32() % (m1 - 1) + 1);
        obj->y[i] = (int32_t) (intf->get_seed32() % (m2 - 1) + 1);
    }
    return obj;
}


MAKE_UINT32_PRNG("CombMRG96", NULL)
