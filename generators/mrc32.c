#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t ctr;
} Mrc32State;


static inline uint64_t get_bits_raw(Mrc32State *obj)
{
    const uint32_t old = obj->a * 0x7f4a7c15;
    obj->a = obj->b + obj->ctr++;
    obj->b = rotl32(obj->b, 19) ^ old;
    return old + obj->a;
}


static void *create(const CallerAPI *intf)
{
    Mrc32State *obj = intf->malloc(sizeof(Mrc32State));
    obj->a = intf->get_seed32();
    obj->b = intf->get_seed32();
    obj->ctr = intf->get_seed32();
    return obj;
}


MAKE_UINT32_PRNG("Mrc32", NULL)



