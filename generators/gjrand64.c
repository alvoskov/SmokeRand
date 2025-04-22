/**
 * @details
 * 1. https://sourceforge.net/p/gjrand/discussion/446985/thread/3f92306c58/
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
} Gjrand64State;


static uint64_t get_bits_raw(void *state)
{
    Gjrand64State *obj = state;
	obj->b += obj->c; obj->a = rotl64(obj->a, 32); obj->c ^= obj->b;
	obj->d += 0x55aa96a5;
	obj->a += obj->b; obj->c = rotl64(obj->c, 23); obj->b ^= obj->a;
	obj->a += obj->c; obj->b = rotl64(obj->b, 19); obj->c += obj->a;
	obj->b += obj->d;
	return obj->a;
}


static void Gjrand64State_init(Gjrand64State *obj, uint64_t seed)
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
    Gjrand64State *obj = intf->malloc(sizeof(Gjrand64State));
    Gjrand64State_init(obj, intf->get_seed64());
    return obj;
}

MAKE_UINT64_PRNG("gjrand64", NULL)
