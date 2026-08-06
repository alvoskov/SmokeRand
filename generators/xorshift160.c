/**
 * @file xorshift160.c
 * @brief An implementation of 160-bit LSFR generator proposed by G. Marsaglia.
 * @details 
 *
 * References:
 * 
 * - Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *   V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 *
 * Notes about shifts triples from Marsaglia article:
 *
 * - [7,13,6] - fairly good
 * - [2,1,4] and [1,1,20] - bad, bspace/gap tests failures at brief.
 *
 * Some triples found by A.L. Voskov:
 *
 * Acceptable: [19 3 11]
 * - brief(3/4), default(6/7), full(9/10)
 * - passes smallcrush, crush:only mrank/lincomp failures
 *
 * Bad: [1 3 4], [5 1 1], [5 3 27], [5 3 29], [2,3,7],
 * [21,7,3], [7,23,2].
 *
 * Medicore: [13 9 3] (smallcrush/crush failures, maxoft)
 *
 * @copyright The xorshift160 algorithm was suggested by G. Marsaglia.
 *
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
    uint32_t w;
    uint32_t v;
} Xorshift160State;


// good: [13 9 3](at brief(3/4), default(6/7), full(9/10), maxoft/smallcrush-????, crush - also bad! )
//       [19 3 11](at brief(3/4), default(6/7), full(9/10) ) : passes smallcrush!, crush:only mrank/lincomp
static inline uint64_t get_bits_raw(Xorshift160State *obj)
{
    uint32_t t = obj->x ^ (obj->x << 19); // a
    t ^= t >> 3; // b
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = obj->v;
    obj->v = (obj->v ^ (obj->v >> 11)) ^ t; // c
    return obj->z;
}


static void *create(const CallerAPI *intf)
{
    Xorshift160State *obj = intf->malloc(sizeof(Xorshift160State));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    obj->z = intf->get_seed32();
    obj->w = intf->get_seed32();
    obj->v = intf->get_seed32();
    if (obj->v == 0) { // State mustn't be all zeros
        obj->v = 0xDEADBEEF;
    }        
    return obj;
}

MAKE_UINT32_PRNG("Xorshift160", NULL)
