/**
 * @file gjrand8.c
 * @brief Implementation of gjrand8 nonlinear chaotic generator.
 * @details It is a modification of gjrand algorithm suggested by M. O'Neill
 * for testing purposes. The gjrand algorithm is designed by D. Blackman
 * (aka G. Jones).
 *
 * References:
 *
 * 1. https://sourceforge.net/p/gjrand/discussion/446985/thread/3f92306c58/
 * 2. https://gist.github.com/imneme/7a783e20f71259cc13e219829bcea4ac
 *
 * @copyright The gjrand8 algorithm is designed by M. O'Neill and D. Blackman
 * (aka G. Jones). Reentrant implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
} Gjrand8State;


static uint8_t Gjrand8State_get_bits(Gjrand8State *obj)
{
	obj->b = (uint8_t) (obj->b + obj->c); // Part 1
    obj->a = rotl8(obj->a, 4);
    obj->c = (uint8_t) (obj->c ^ obj->b);
	obj->d = (uint8_t) (obj->d + 0x35U);  // Part 2
	obj->a = (uint8_t) (obj->a + obj->b); // Part 3
    obj->c = rotl8(obj->c, 2);
    obj->b = (uint8_t) (obj->b ^ obj->a);
	obj->a = (uint8_t) (obj->a + obj->c); // Part 4
    obj->b = rotl8(obj->b, 5);
    obj->c = (uint8_t) (obj->c + obj->a);
	obj->b = (uint8_t) (obj->b + obj->d); // Part 5
	return obj->a;
}

static inline uint64_t get_bits_raw(void *state)
{
    union {
        uint8_t  u8[4];
        uint32_t u32;
    } out;
    for (int i = 0; i < 4; i++) {
        out.u8[i] = Gjrand8State_get_bits(state);
    }
    return out.u32;
}


static void Gjrand8State_init(Gjrand8State *obj, uint8_t seed)
{
    obj->a = seed;
    obj->b = 0;
    obj->c = 201;
    obj->d = 0;
    for (int i = 0; i < 14; i++) {
        (void) Gjrand8State_get_bits(obj);
    }
}


static void *create(const CallerAPI *intf)
{
    Gjrand8State *obj = intf->malloc(sizeof(Gjrand8State));
    Gjrand8State_init(obj, (uint8_t) intf->get_seed64());
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t u_ref[4] = {
        0x48C49B99, 0xF143EB7D, 0xADE11E34, 0xEA7760E1
    };
    Gjrand8State obj;
    Gjrand8State_init(&obj, 0x12);
    int is_ok = 1;
    for (size_t i = 0; i < 4; i++) {
        const uint32_t u = (uint32_t) get_bits_raw(&obj);
        intf->printf("Out = %8.8" PRIX32 "; ref = %8.8" PRIX32 "\n",
            u, u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }    
    return is_ok;
}

MAKE_UINT32_PRNG("gjrand8", run_self_test)
