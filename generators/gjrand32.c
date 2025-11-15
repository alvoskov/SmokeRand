/**
 * @file gjrand32.c
 * @brief Implementation of gjrand64 nonlinear chaotic generator.
 * @details The gjrand32 algorithm is designed by D. Blackman (aka G. Jones),
 * its period is at least \f$ 2^{32} \f$.
 *
 * References:
 *
 * 1. https://sourceforge.net/p/gjrand/discussion/446985/thread/3f92306c58/
 * 2. https://gist.github.com/imneme/7a783e20f71259cc13e219829bcea4ac
 *
 * @copyright The gjrand64 algorithm is designed by D. Blackman (aka G. Jones).
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief gjrand64 generator state.
 */
typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
} Gjrand32State;


static uint32_t get_bits_raw(void *state)
{
    Gjrand32State *obj = state;
	obj->b += obj->c; obj->a = rotl32(obj->a, 16); obj->c ^= obj->b;
	obj->d += 0x96a5;
	obj->a += obj->b; obj->c = rotl32(obj->c, 11); obj->b ^= obj->a;
	obj->a += obj->c; obj->b = rotl32(obj->b, 19); obj->c += obj->a;
	obj->b += obj->d;
	return obj->a;
}


static void Gjrand32State_init(Gjrand32State *obj, uint32_t seed)
{
    obj->a = seed;
    obj->b = 0;
    obj->c = 2000001;
    obj->d = 0;
    for (int i = 0; i < 14; i++) {
        (void) get_bits_raw(obj);
    }
}


static void *create(const CallerAPI *intf)
{
    Gjrand32State *obj = intf->malloc(sizeof(Gjrand32State));
    Gjrand32State_init(obj, intf->get_seed32());
    return obj;
}

MAKE_UINT32_PRNG("gjrand32", NULL)
