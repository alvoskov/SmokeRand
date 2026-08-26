#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t x;
    uint32_t w;
} Xormix32State;


static inline uint64_t get_bits_raw(Xormix32State *obj)
{
    obj->w += 0xAC6D9BB7U;
    obj->x = (obj->x ^ rotl32(obj->x, 5) ^ rotl32(obj->x, 24)) + obj->w;
    return obj->x;
}

static void *create(const CallerAPI *intf)
{
    Xormix32State *obj = intf->malloc(sizeof(Xormix32State));
    obj->x = intf->get_seed32();
    obj->w = intf->get_seed32();
    for (int i = 0; i < 32; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("xormix32", NULL)


// 