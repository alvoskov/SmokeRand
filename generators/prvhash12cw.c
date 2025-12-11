// https://github.com/avaneev/prvhash
// 
#include "smokerand/cinterface.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t seed;
    uint16_t lcg;
    uint16_t hash;
    uint16_t w;
} PrvHashCore16State;


static inline uint16_t PrvHashCore16State_get_bits(PrvHashCore16State *obj)
{
    obj->w = (uint16_t) ((obj->w + 0x9E3) & 0xFFF);
    obj->seed *= (uint16_t) ( (obj->lcg * 2U + 1U) & 0xFFF);
	const uint16_t rs = ((obj->seed << 6) | (obj->seed >> 6)) & 0xFFF;
    obj->hash  = (uint16_t) ((obj->hash + rs + 0xAAA) & 0xFFF);
    obj->lcg   = (uint16_t) ((obj->lcg + obj->seed + obj->w) & 0xFFF);
    obj->seed ^= obj->hash;
    return obj->lcg ^ rs;
}

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t a = PrvHashCore16State_get_bits(state);
    uint32_t b = PrvHashCore16State_get_bits(state);
    uint32_t c = PrvHashCore16State_get_bits(state);
    return (a << 24) | (b << 12) | c;
}

static void *create(const CallerAPI *intf)
{
    PrvHashCore16State *obj = intf->malloc(sizeof(PrvHashCore16State));
    obj->seed = (uint16_t) intf->get_seed64() & 0xFFF;
    obj->lcg  = (uint16_t) intf->get_seed64() & 0xFFF;
    obj->hash = (uint16_t) intf->get_seed64() & 0xFFF;
    obj->w    = (uint16_t) intf->get_seed64() & 0xFFF;
    return obj;
}



MAKE_UINT32_PRNG("prvhash-core12-weyl", NULL)
