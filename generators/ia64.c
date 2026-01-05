/** 
 * @file ia64.c
 * @brief IA nonlinear generator by Bob Jenkins.
 * @details It is a nonlinear generator based on ideas from RC4, the 64-bit
 * modification is made by A.L. Voskov.
 *
 * References:
 *
 * 1. https://burtleburtle.net/bob/rand/isaac.html#IAcode
 *
 * @copyright
 * Based on public domain code by Bob Jenkins.
 *
 * Adaptation for SmokeRand:
 *
 * @copyright
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


#define ALPHA      (8)
#define SIZE       (1<<ALPHA)
#define ind(x)     (x&(SIZE-1))

typedef struct {
    uint64_t m[SIZE]; ///< Memory: array of SIZE ALPHA-bit terms
    uint64_t bb; ///< Previous result
    int i;
} Ia64State;


static inline uint64_t get_bits_raw(Ia64State *obj)
{
    const uint64_t x = obj->m[obj->i];
    const uint64_t y = obj->m[ind(x)] + obj->bb;
    obj->m[obj->i] = y;                     // set m
    const uint64_t r = obj->m[ind(y>>ALPHA)] + x; // set r
    obj->bb = r;
    obj->i++;
    obj->i = ind(obj->i);
    return r;
}


static void *create(const CallerAPI *intf)
{
    Ia64State *obj = intf->malloc(sizeof(Ia64State));
    uint64_t seed = intf->get_seed64();
    obj->i = 0;
    obj->bb = 0;
    for (int i = 0; i < SIZE; i++) {
        obj->m[i] = (uint32_t) pcg_bits64(&seed);
    }
    return obj;
}


MAKE_UINT64_PRNG("IA64", NULL)
