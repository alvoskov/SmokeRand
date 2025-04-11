/**
 * @file tylo64.c
 * @brief Tylo64 is a modification of SFC64 suggested by Tyge Løvset.
 * @details It uses a reduced state and arbitrary odd increment for the
 * counter ("discrete Weyl sequence") to implement threads. However, this
 * version for SmokeRand always uses increment 1.
 *
 * References:
 *
 * 1. https://github.com/numpy/numpy/issues/16313#issuecomment-641897028
 * 2. https://lemire.me/blog/2019/03/19/the-fastest-conventional-random-number-generator-that-can-pass-big-crush/
 *
 * @copyright SFC64 algorithm is developed by Chris Doty-Humphrey,
 * the author of PractRand (https://sourceforge.net/projects/pracrand/).
 * The Tylo64 modification was suggested by Tyge Løvset.
 * 
 * Adaptation for SmokeRand:
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief tylo64 state.
 */
typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t counter;
} Tylo64State;


static inline uint64_t get_bits_raw(void *state)
{
    Tylo64State *obj = state;
    const uint64_t b = obj->b, out = obj->a ^ obj->counter++;
    obj->a = (b + (b << 3)) ^ (b >> 11);
    obj->b = ((b << 24) | (b >> 40)) + out;
    return out;
}


static void Tylo64State_init(Tylo64State *obj, uint64_t s0, uint64_t s1)
{
    obj->a = s0;
    obj->b = s1;
    obj->counter = 2;
    for (int i = 0; i < 64; i++) {
        (void) get_bits_raw(obj);
    }
}


static void *create(const CallerAPI *intf)
{
    Tylo64State *obj = intf->malloc(sizeof(Tylo64State));    
    uint64_t s0 = intf->get_seed64();
    uint64_t s1 = intf->get_seed64();
    Tylo64State_init(obj, s0, s1);
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref = 0x8DF0BE72825CB80E;
    Tylo64State obj;
    Tylo64State_init(&obj, 3, 2);
    uint64_t u = get_bits_raw(&obj);
    intf->printf("Output: %llX; reference: %llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;    
}

MAKE_UINT64_PRNG("Tylo64", run_self_test)
