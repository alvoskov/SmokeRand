// https://github.com/tommyettinger/juniper/blob/main/src/main/java/com/github/tommyettinger/random/Chip32Random.java


#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
} Chip32State;


static inline uint64_t get_bits_raw(Chip32State *obj)
{
    const uint32_t a = obj->a, b = obj->b, c = obj->c, d = obj->d;
    obj->a = b + c;
    obj->b = d ^ a;
    obj->c = rotl32(b, 11);
    obj->d += 0x9E3779B9U;
    return rotl32(a, 14) ^ (rotl32(b, 23) + c);
}

static void *create(const CallerAPI *intf)
{
    Chip32State *obj = intf->malloc(sizeof(Chip32State));
    obj->a = intf->get_seed32();
    obj->b = intf->get_seed32();
    obj->c = intf->get_seed32();
    obj->d = intf->get_seed32();
    return obj;
}

MAKE_UINT32_PRNG("chip32", NULL)
