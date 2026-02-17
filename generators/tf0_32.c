#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


static inline uint64_t get_bits_raw(Lcg32State *obj)
{
    obj->x += obj->x * obj->x | 5;
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Lcg32State *obj = intf->malloc(sizeof(Lcg32State));
    obj->x = intf->get_seed32();
    return obj;
}


MAKE_UINT32_PRNG("tf0_32", NULL)
