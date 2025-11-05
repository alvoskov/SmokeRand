/**
 * @details
 * 1. https://sourceforge.net/p/gjrand/discussion/446985/thread/3f92306c58/
 * 2. https://gist.github.com/imneme/7a783e20f71259cc13e219829bcea4ac
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
	obj->b += obj->c; obj->a = rotl16(obj->a, 8); obj->c ^= obj->b;
	obj->d += 0x96A5;
	obj->a += obj->b; obj->c = rotl16(obj->c, 5); obj->b ^= obj->a;
	obj->a += obj->c; obj->b = rotl16(obj->b, 10); obj->c += obj->a;
	obj->b += obj->d;
	return obj->a;
}

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t hi = Gjrand16State_get_bits(state);
    uint32_t lo = Gjrand16State_get_bits(state);
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

MAKE_UINT32_PRNG("gjrand16", NULL)
