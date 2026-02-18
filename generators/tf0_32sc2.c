#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


static inline uint64_t get_bits_raw(Lcg32State *obj)
{
    const uint32_t out = (obj->x ^ (obj->x >> 16)) * 69069U;
    obj->x += obj->x * obj->x | 0x4005;
    return out ^ rotl32(out, 7) ^ rotl32(out, 23);
}


static void *create(const CallerAPI *intf)
{
    Lcg32State *obj = intf->malloc(sizeof(Lcg32State));
    obj->x = intf->get_seed32();
    return obj;
}


MAKE_UINT32_PRNG("tf0_32sc2", NULL)
