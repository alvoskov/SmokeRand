/**
 * @file mzran13.c
 * @brief A combined PRNG suggested by Marsaglia and Zaman.
 * @details This combined generator is probably better than `69069` but still
 * obsolete and fails a lot of tests, especially in its lower bits.
 *
 * References:
 *
 * 1. Marsaglia G., Zaman A. Some portable very-long-period random number
 *    generators // Comput. Phys. 1994. V. 8. N 1. P. 117-121.
 *    https://doi.org/10.1063/1.168514.
 *
 * @copyright The original algorithm was suggested by G. Marsaglia and A. Zaman.
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MZRAN13 combined PRNG state
 */
typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t c;
    uint32_t n;
} Mzran13State;


static uint32_t get_bits_raw(Mzran13State *obj)
{
    uint32_t s;
    if (obj->y > obj->x + obj->c) {
        s = obj->y - (obj->x + obj->c);      obj->c = 0;
    } else {
        s = obj->y - (obj->x + obj->c) - 18; obj->c = 1;
    }
    obj->x = obj->y; obj->y = obj->z; obj->z = s;
    obj->n = 69069u * obj->n + 1013904243u;
    return obj->z + obj->n;    
}


static void *create(const CallerAPI *intf)
{
    Mzran13State *obj = intf->malloc(sizeof(Mzran13State));
    const uint64_t seed0 = intf->get_seed64();
    const uint64_t seed1 = intf->get_seed64();
    obj->x = seed0 & 0x7FFFFFFF;
    obj->y = (seed0 >> 32) & 0x7FFFFFFF;
    obj->z = seed1 & 0x7FFFFFFF;
    obj->c = 1;
    obj->n = (uint32_t) (seed1 >> 32);
    return obj;
}

MAKE_UINT32_PRNG("Mzran13", NULL)
