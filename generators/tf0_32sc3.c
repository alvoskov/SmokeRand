#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


static inline uint64_t get_bits_raw(Lcg32State *obj)
{
    uint32_t out = obj->x;
    out = out ^ rotl32(out, 7) ^ rotl32(out, 23);
    out = rotl32(69069U * out, 5);
    out += out * out | 0x5;
    obj->x += obj->x * obj->x | 0x4005;
    return out;
}


static void *create(const CallerAPI *intf)
{
    Lcg32State *obj = intf->malloc(sizeof(Lcg32State));
    obj->x = intf->get_seed32();
    return obj;
}


MAKE_UINT32_PRNG("tf0_32sc2", NULL)
