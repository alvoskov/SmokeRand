/**
 * @file gjrand16.c
 * @brief Implementation of gjrand16 nonlinear chaotic generator.
 * @details It is a modification of gjrand algorithm suggested by M. O'Neill
 * for testing purposes. The gjrand algorithm is designed by D. Blackman
 * (aka G. Jones).
 *
 * References:
 *
 * 1. https://sourceforge.net/p/gjrand/discussion/446985/thread/3f92306c58/
 * 2. https://gist.github.com/imneme/7a783e20f71259cc13e219829bcea4ac
 *
 * @copyright The gjrand16 algorithm is designed by M. O'Neill and D. Blackman
 * (aka G. Jones). Reentrant implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t a;
    uint16_t b;
    uint16_t c;
    uint16_t d;
} Gjrand16State;


static uint16_t Gjrand16State_get_bits(Gjrand16State *obj)
{
	obj->b = (uint16_t) (obj->b + obj->c); // Part 1
    obj->a = rotl16(obj->a, 8);
    obj->c = (uint16_t) (obj->c ^ obj->b);

	obj->d = (uint16_t) (obj->d + 0x96A5U); // Part 2

	obj->a = (uint16_t) (obj->a + obj->b); // Part 3
    obj->c = rotl16(obj->c, 5);
    obj->b = (uint16_t) (obj->b ^ obj->a);

	obj->a = (uint16_t) (obj->a + obj->c); // Part 4
    obj->b = rotl16(obj->b, 10);
    obj->c = (uint16_t) (obj->c + obj->a);

	obj->b = (uint16_t) (obj->b + obj->d); // Part 5
	return obj->a;
}

static inline uint64_t get_bits_raw(void *state)
{
    const uint32_t hi = Gjrand16State_get_bits(state);
    const uint32_t lo = Gjrand16State_get_bits(state);
    return (hi << 16) | lo;
}


static void Gjrand16State_init(Gjrand16State *obj, uint16_t seed)
{
    obj->a = seed;
    obj->b = 0;
    obj->c = 2001;
    obj->d = 0;
    for (int i = 0; i < 14; i++) {
        (void) Gjrand16State_get_bits(obj);
    }
}


static void *create(const CallerAPI *intf)
{
    Gjrand16State *obj = intf->malloc(sizeof(Gjrand16State));
    Gjrand16State_init(obj, (uint16_t) intf->get_seed64());
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t u_ref[4] = {
        0x59417EE0, 0x87DA95F6, 0x18759DE6, 0x3B6D29F4
    };
    Gjrand16State obj;
    Gjrand16State_init(&obj, 0x1234);
    int is_ok = 1;
    for (size_t i = 0; i < 4; i++) {
        const uint32_t u = (uint32_t) get_bits_raw(&obj);
        intf->printf("Out = %8.8lX; ref = %8.8lX\n",
            (unsigned long) u, (unsigned long) u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    
    return is_ok;
}

MAKE_UINT32_PRNG("gjrand16", run_self_test)
