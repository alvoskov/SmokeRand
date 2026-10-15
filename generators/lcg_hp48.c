#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    __uint128_t x;
} HP48State;

static inline uint64_t get_bits_raw(HP48State *obj)
{
    obj->x = (2851130928467U * obj->x) % 1000000000000000U;
    uint32_t out = (uint32_t) ((double) obj->x * 4.294967296e-06);
    return out;
}


static void *create(const CallerAPI *intf)
{
    HP48State *obj = intf->malloc(sizeof(HP48State));
    
    obj->x = intf->get_seed64() | 7;
    return obj;
}


MAKE_UINT32_PRNG("LCG_HP48", NULL)
