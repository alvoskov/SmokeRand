/**
 * @file xorshift96.c
 * @brief An implementation of 96-bit LSFR generator proposed by G. Marsaglia.
 * @details 
 *
 * References:
 * 
 * - Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *   V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 *
 * Notes about shifts triples:
 *
 * - [13, 19, 3] - good bspace in brief, 4/5 tests failed
 * - [10, 5, 26], [1, 17, 2] and [10, 1, 26] fail bspace and/or gap tests.
 *
 * @copyright The xorshift96 algorithm was suggested by G. Marsaglia.
 * Adaptation for SmokeRand and selection of fairly good shifts triple:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Xorshift96 PRNG state
 */
typedef struct {
    uint32_t x;
    uint32_t y; 
    uint32_t z;
} Xorshift96State;


static inline uint64_t get_bits_raw(Xorshift96State *obj)
{
    uint32_t t = obj->x ^ (obj->x << 13); // a
    t ^= t >> 19; // b
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = (obj->z ^ (obj->z >> 3)) ^ t; // c
    return obj->z;
}


static void *create(const CallerAPI *intf)
{
    Xorshift96State *obj = intf->malloc(sizeof(Xorshift96State));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    obj->z = intf->get_seed32();
    if (obj->z == 0) { // State mustn't be all zeros
        obj->z = 0xDEADBEEF;
    }        
    return obj;
}

MAKE_UINT32_PRNG("Xorshift96", NULL)
