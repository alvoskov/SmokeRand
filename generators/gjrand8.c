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

PRNG_CMODULE_PROLOG

typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
} Gjrand8State;


static uint8_t Gjrand8State_get_bits(Gjrand8State *obj)
{
	obj->b += obj->c; obj->a = rotl8(obj->a, 4); obj->c ^= obj->b; // Line 1
	obj->d += 0x35; // Line 2
	obj->a += obj->b; obj->c = rotl8(obj->c, 2); obj->b ^= obj->a; // Line 3
	obj->a += obj->c; obj->b = rotl8(obj->b, 5); obj->c += obj->a; // Line 4
	obj->b += obj->d; // Line 5
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

MAKE_UINT32_PRNG("gjrand8", NULL)
