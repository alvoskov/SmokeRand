// https://github.com/avaneev/prvhash
// 
#include "smokerand/cinterface.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t seed;
    uint16_t lcg;
    uint16_t hash;
} PrvHashCore16State;


static inline uint16_t PrvHashCore16State_get_bits(PrvHashCore16State *obj)
{
    obj->seed *= (uint16_t) (obj->lcg * 2U + 1U);
	const uint16_t rs = rotl16(obj->seed, 8);
    obj->hash += (uint16_t) (rs + 0xAAAA);
    obj->lcg += (uint16_t) (obj->seed + 0x5555);
    obj->seed ^= obj->hash;
    return obj->lcg ^ rs;
}

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t hi = PrvHashCore16State_get_bits(state);
    uint32_t lo = PrvHashCore16State_get_bits(state);
    return (hi << 16) | lo;
}

static void *create(const CallerAPI *intf)
{
    PrvHashCore16State *obj = intf->malloc(sizeof(PrvHashCore16State));
    obj->seed = (uint16_t) intf->get_seed64();
    obj->lcg  = (uint16_t) intf->get_seed64();
    obj->hash = (uint16_t) intf->get_seed64();
    return obj;
}



MAKE_UINT32_PRNG("prvhash-core16", NULL)
