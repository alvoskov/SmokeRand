/**
 * @details
 * 1. https://sourceforge.net/p/gjrand/discussion/446985/thread/3f92306c58/
 * 2. https://gist.github.com/imneme/7a783e20f71259cc13e219829bcea4ac
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
} Gjrand8State;

static inline uint8_t rotl8(uint8_t x, int r)
{
    return (uint8_t) ( (x << r) | (x >> (8 - r)) );
}


static uint8_t Gjrand8State_get_bits(Gjrand8State *obj)
{
	obj->b += obj->c; obj->a = rotl8(obj->a, 4); obj->c ^= obj->b;
	obj->d += 0x35;
	obj->a += obj->b; obj->c = rotl8(obj->c, 2); obj->b ^= obj->a;
	obj->a += obj->c; obj->b = rotl8(obj->b, 5); obj->c += obj->a;
	obj->b += obj->d;
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
