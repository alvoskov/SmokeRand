/**
 * @file cwg64_shared.c
 * @brief "Collatz-Weyl" generator.
 * @details https://arxiv.org/pdf/2312.17043
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {    
    uint64_t x;
    uint64_t a;
    uint64_t w;
} Cwg64State;

static inline uint64_t get_bits_raw(void *state)
{    
    Cwg64State *obj = state;
    obj->w += 0x9E3779B97F4A7C15;
    obj->a += obj->x;
    obj->x = (obj->x >> 1) * (obj->x | 1) ^ obj->w;
    return (obj->a >> 48) ^ obj->x;
}

static void *create(const CallerAPI *intf)
{
    Cwg64State *obj = intf->malloc(sizeof(Cwg64State));
    obj->x = intf->get_seed64();
    obj->a = intf->get_seed64();
    obj->w = intf->get_seed64();
    for (int i = 0; i < 48; i++) {
        (void) get_bits_raw(obj);
    }
    return (void *) obj;
}

MAKE_UINT64_PRNG("CWG64", NULL)
