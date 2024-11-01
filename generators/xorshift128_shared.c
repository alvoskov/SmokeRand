/**
 * @file xorshift128_shared.c
 * @brief An implementation of 128-bit LSFR generator proposed by G. Marsaglia.
 * @details 
 *
 * References:
 * 
 * - Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *   V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Xorshift128 PRNG state
 */
typedef struct {
    uint32_t x;
    uint32_t y; 
    uint32_t z;
    uint32_t w;
} Xorshift128State;


static inline uint64_t get_bits_raw(void *state)
{
    Xorshift128State *obj = state;
    uint32_t t = (obj->x ^ (obj->x << 11));
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = (obj->w ^ (obj->w >> 19)) ^ ( t ^ (t >> 8) );
    return obj->w;
}


static void *create(const CallerAPI *intf)
{
    Xorshift128State *obj = intf->malloc(sizeof(Xorshift128State));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    obj->z = intf->get_seed32();
    obj->w = intf->get_seed32() | 0x1; // State mustn't be all zeros
    return (void *) obj;
}

MAKE_UINT32_PRNG("Xorshift128", NULL)
